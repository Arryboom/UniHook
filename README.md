# UniHook

This is an example project on how to use PolyHook to intercept ANY arbitrary function, without knowing it's typedef. It hooks the specified function and calls an "Interupt" function before and after executing the original hooked function. This is useful for cases where someone may wish to time how long a function takes to execute. This relies both on PolyHook, it's dependancy Capstone (the modified branch in my GitHub).

#How it works:

It allocates a callback at runtime that PolyHook redirects the hooked function to. This callback is filled with assembly at runtime that will handle executing the intercepts, and the original function. This is done by first storing all registers, calling the intercept, and restoring the registers, then it calls the original function, and repeats the register storing and poping before it calls the second interupt and returns to the caller of the hooked function.

Caller->Store Regs->Interupt1->Restore Regs->Original Hooked Func->Store Regs->Interupt2->Restore Regs->Return to Caller

#Project Setup

This demo has 3 core parts

1. PolyHook for hooking backend
2. UniHook dll which uses polyhook, and then creates runtime callbacks
3. UniHook loader which injects the UniHook dll, and sends it commands via a shared memory queue/stack system

#LICENSE
MIT
