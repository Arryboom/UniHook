# UniHook

This is an example project on how to use PolyHook to intercept ANY arbitrary function, without knowing it's typedef. It hooks the specified function and calls an "Interupt" function before and after executing the original hooked function. This is useful for cases where someone may wish to time how long a function takes to execute. This relies both on PolyHook, it's dependancy Capstone (the modified branch in my GitHub).
