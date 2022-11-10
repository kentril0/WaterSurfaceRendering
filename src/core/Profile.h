
#ifndef WATER_SURFACE_RENDERING_CORE_PROFILE_H_
#define WATER_SURFACE_RENDERING_CORE_PROFILE_H_

#include <chrono>
#include <unordered_map>

#define MICRO_TO_MILLIS 0.001

// Profiling macros
// use VKP_PROFILE_SCOPE() , or
// use VKP_PROFILE_SCOPE("your description") 

namespace vkp
{
    /**
     * @brief Singleton for global profiling:
     *  a) Keeps a global data structure of profiling records.
     *  b) Profiling records can be inserted into the data structure from
     *      anywhere using compile-time macros:
     *      {
     *          VKP_PROFILE_SCOPE() // (1)
     *          ...
     *      }   // The time taken from (1) to here is recorded.
     *
     *  c) The global data structure keeps each record once and keeps track of
     *      their ordering, so that the most recently added are extracted from
     *      "the front". That is, each next record inserted with the same string
     *      literal has its duration updated and "its position is bumped to the
     *      front".
     *  d) The data structure is an std::unordered_map with internal record type
     *      implementing doubly-linked list to keep track of only the unique
     *      records and the most recently inserted. *The key must be a string
     *      literal*, this allows for O(const) insertion time thanks to a single
     *      value comparison, along with a simple correction of pointers between
     *      the linked records.
     *  e) The records, in order of the most recently inserted, can be obtained
     *      using the "GetRecordsFromLatest" function, that implements a "walk"
     *      through the internal doubly-linked list in the data structure 
     *      in O(n) time.
     */
    class Profile
    {
    public:

        struct Record
        {
            const char* name;
            float duration;
            
            Record(const char* str, float val) : name(str), duration(val) {}

            bool operator==(const Record& o) const
            {
                return o.name == name;
            }
        };

        /**
         * @return All records ordered from most recently inserted
         */
        static std::vector<Record> GetRecordsFromLatest();

        static void InsertRecord(const Record& r);
        static void InsertRecord(const char* desc, float val);

        template<typename Fn>
        class Timer 
        {
        public:
            /**
             * @param name Name of the timed event
             * @param func Called when stopped with parameter of type Profile::Record
             */
            Timer(const char* name, Fn&& func)
                : m_Name(name), m_Stopped(false), m_Func(func) { Start(); }

            ~Timer()
            {
                if (!m_Stopped)
                    Stop();
            }
        private:
            inline void Start()
            {
                m_Start = std::chrono::high_resolution_clock::now();
            }

            void Stop()
            {
                m_Stopped = true;
                m_Func({ m_Name, ElapsedMillis() });
            }

            inline float ElapsedMillis() const
            {
                return std::chrono::duration_cast<std::chrono::microseconds>
                    (std::chrono::high_resolution_clock::now() - m_Start).count() *
                    MICRO_TO_MILLIS;
            }

        private:
            const char* m_Name;
            bool m_Stopped;
            Fn m_Func;

            std::chrono::time_point<std::chrono::high_resolution_clock> m_Start;
        };

    private:
        struct InternalRecord
        {
            const char* name;
            float duration;

            InternalRecord* prev;
            InternalRecord* next;

            InternalRecord() : InternalRecord("Undef", 0.0f) {}
            InternalRecord(const char* desc, float val)
                : name(desc), duration(val), prev{nullptr}, next{nullptr} {}

            bool operator==(const InternalRecord& o) const
            {
                return o.name == name;
            }
        };

        static inline std::unordered_map<const char*, InternalRecord> s_Records;
        static inline InternalRecord* s_LatestRecord{ nullptr };
    };

    /**
     * @return Base file name from a file path string literal,
     *  gets expanded at compile-time.
     *  Tested with x86-64 versions of: gcc 12.2, clang 15.0, MSVC 19.0
     */
    constexpr const char* GetBaseName(const char* path)
    {
        const char* p = path;
        while (*path != '\0')
        {
            if (*path == '/' || *path == '\\')
                p = path;

            ++path;
        }
        return ++p;
    }
    
} // namespace vkp


#define VKP_PSCOPE1(name) vkp::Profile::Timer timer##__LINE__( \
    name, \
    [&](vkp::Profile::Record r) { vkp::Profile::InsertRecord(r); } \
)

#define VKP_PSCOPE0() vkp::Profile::Timer timer1##__LINE__( \
    __func__, \
    [&](vkp::Profile::Record r) { vkp::Profile::InsertRecord(r); } \
)

#define VKP_EXPAND_PSCOPE(_0,_1,fname, ...) fname

#define VKP_PROFILE_SCOPE(...) \
    VKP_EXPAND_PSCOPE(_0, ##__VA_ARGS__, VKP_PSCOPE1, VKP_PSCOPE0)(__VA_ARGS__)


#endif // WATER_SURFACE_RENDERING_CORE_PROFILE_H_
