#if defined(ENTITY)

#define HOST_(a) CDEF2STR(a)
#define HOST "esp32_" HOST_(ENTITY)

#define ESP32_(a) (ENTITY == (a))

/*
 *  #if ESP32_(0)
 *  ...
 *  #elif ESP32_(35)
 *
 * replaces:
 *
 *  #if defined(ESP32_0)
 *  ...
 *  #elif defined(ESP32_35)
 *
 */
#else

#error no ENTITY given

#endif

