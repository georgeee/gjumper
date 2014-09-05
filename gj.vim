"if !has("perl")
"    finish
"endif
"
let s:plugin_path = escape(expand('<sfile>:p:h'), '\')
let s:default_gjumper_client_path = s:plugin_path . "/gjumper_client"


function! gj#GetPID()
    perl VIM::DoCommand('let s:pid =' . $$)
    return s:pid
endfunction

let s:gjumperPort = 0
call gj#GetPID()

function! gj#ProcessRequest(type, ...)
    let commandBase = !exists("g:gjCommandBase") ? s:default_gjumper_client_path : g:gjCommandBase
    let command = commandBase . " " . s:pid . " " . s:gjumperPort . " " . a:type
    for argument in a:000
        let command = command . " " . argument
    endfor
    echom "Executing command: " . command
    let output = system(command)
    if (v:shell_error != 0)
        return 0
    endif
    let obj = ParseJSON(output)
    let s:gjumperPort = obj.port
    if obj["status"] != 1
        return 0
    endif
    return has_key(obj, "data") ? obj.data : 1
endfunction

function! gj#ResolveLine(...)
    let file = a:0 >= 1 ? a:1 : @%
    let line = a:0 >= 2 ? a:2 : line(".")
    let res = gj#ProcessRequest("resolve", file, line)
    return (type(res) == type(0) ? [] : res)
endfunction

function! gj#Resolve(...)
    let file = a:0 >= 1 ? a:1 : @%
    let line = a:0 >= 2 ? a:2 : line(".")
    let col  = a:0 >= 3 ? a:3 : col(".")
    return gj#ProcessRequest("resolve", file, line, col)
endfunction

function! gj#FullRecache()
    return gj#ProcessRequest("full_recache")
endfunction

function! gj#RecacheFile(...)
    let file = a:0 >= 1 ? a:1 : @%
    return gj#ProcessRequest("recache", file)
endfunction

function! gj#JumpToFile(file, line, col)
    execute "o " . a:file
    execute "norm! " . a:line . "G" . a:col . "|"
endfunction

function! gj#ShowResolveList(hint_list)
    if type(a:hint_list) != type([]) || a:hint_list == []
        return 0
    endif
    
    let choices = []
    let i = 0
    let s:hint_list = a:hint_list
    for hint in a:hint_list
        let str = hint[2] . " -> " . hint[3]
                    \ . " (" . hint[1][0]
                    \ . ":" . hint[1][1]
                    \ . ":" . hint[1][2] . ")"
        if hint[5] != "reg"
            let str = str . " [" . hint[5] . "]"
        endif
        call add(choices, [str, i]) 
        let i = i + 1
    endfor
    function! GjListSelectAction(...) dict
        call g:forms#submitAction.execute()
    endfunction
    let action = forms#newAction({ 'execute' : function("GjListSelectAction")})
    let attrs = { 'choices' : choices,  'tag' : 'gjSList',
                \ 'on_selection_action' : action}
    let popdownlist2 = forms#newSelectList(attrs)
    let box2 = forms#newBox({ 'body': popdownlist2 })
    let form = forms#newForm({'body': box2})
    let choice = form.run()
    if type(choice) != type(0)
        let hint = s:hint_list[choice.gjSList[1]]
        return gj#JumpToFile(hint[1][0], hint[1][1], hint[1][2])
    endif
endfunction

function! gj#ShowLineHints()
    return gj#ShowResolveList(gj#ResolveLine())
endfunction

function! gj#TriggerBufWrite(file)
    return gj#RecacheFile(a:file)
endfunction

nnoremap <silent> <F8> :call gj#ShowLineHints()<CR>
nnoremap <silent> <Leader><F8> :call gj#RecacheFile()<CR>
nnoremap <silent> <Leader><F7> :call gj#FullRecache()<CR>

"Registering onWrites handlers

"@TODO getList of indexed files, foreach setHandler
"Think of how to update handlers
augroup gjumper
    au!
    au BufWrite *.cpp call gj#TriggerBufWrite(expand("%"))
    au BufWrite *.c   call gj#TriggerBufWrite(expand("%"))
    au BufWrite *.h   call gj#TriggerBufWrite(expand("%"))
    au BufWrite *.hpp call gj#TriggerBufWrite(expand("%"))
augroup END
