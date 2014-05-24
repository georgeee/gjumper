"if !has("perl")
"    finish
"endif


function! gj#GetPID()
    perl VIM::DoCommand('let s:pid =' . $$)
    return s:pid
endfunction

let s:gjumperPort = 0
call gj#GetPID()

function! gj#ProcessRequest(type, ...)
    let commandBase = !exists("g:gjCommandBase") ? "../gjumper_client" : g:gjCommandBase
    let command = commandBase . " " . s:pid . " " . s:gjumperPort . " " . a:type
    for argument in a:000
        let command = command . " " . argument
    endfor
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
    return gj#ProcessRequest("resolve", file, line)
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

function! gj#ShowResolvePopdown(hint_list)
    let choices = []
    let s:hint_list = a:hint_list
    
    let i = 0
    for hint in a:hint_list
        call add(choices, [hint[2] . " -> " . hint[3]
                    \ . " (" . hint[1][0]
                    \ . ":" . hint[1][1]
                    \ . ":" . hint[1][2] . ")", i]) 
        let i = i + 1
    endfor
    let attrs = {
                \ 'choices' : choices,
                \ 'size' : 4,
                \ 'pos' : 0
                \ }
    let popdownlist2 = forms#newPopDownList(attrs)
    let box2 = forms#newBox({ 'body': popdownlist2 })
    let form = forms#newForm({'body': box2})
    call form.run()
endfunction

function! gj#ShowLineHints()
    return gj#ShowResolvePopdown(gj#ResolveLine())
endfunction

