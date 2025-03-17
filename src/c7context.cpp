#include <c7context.hpp>

#if defined(USE_C7_CONTEXT)

// c7context.*.s

#else

extern "C" {

int c7_getcontext(c7_context_t ctx)
{
    return ::getcontext(ctx);
}

static void c7_call(uint32_t fl, uint32_t fh, uint32_t pl, uint32_t ph)
{
    uint64_t f64 = (static_cast<uint64_t>(fh)<<32)|fl;
    uint64_t p64 = (static_cast<uint64_t>(ph)<<32)|pl;
    reinterpret_cast<void(*)(void*)>(f64)(reinterpret_cast<void*>(p64));
}

void c7_makecontext(c7_context_t ctx, void (*func)(void*), void *prm)
{
    uint64_t f64 = reinterpret_cast<uint64_t>(func);
    uint64_t p64 = reinterpret_cast<uint64_t>(prm);
    ::makecontext(ctx, reinterpret_cast<void(*)()>(c7_call),
		  4, (f64<<32)>>32, (f64>>32), (p64<<32)>>32, (p64>>32));
}

int c7_swapcontext(c7_context_t c_ctx, const c7_context_t t_ctx)
{
    return ::swapcontext(c_ctx, t_ctx);
}

} // extern "C"

#endif
