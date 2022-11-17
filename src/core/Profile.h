
#ifndef WATER_SURFACE_RENDERING_CORE_PROFILE_H_
#define WATER_SURFACE_RENDERING_CORE_PROFILE_H_

#include <chrono>
#include <unordered_map>

#define MICRO_TO_MILLIS 0.001

/**
 * @brief Define VKP_PROFILE to use global *Profiling macros*:
 * VKP_PROFILE_SCOPE() , or
 * VKP_PROFILE_SCOPE("your description")
 */

#ifdef VKP_PROFILE
    #define VKP_PSCOPE1(name, line) constexpr auto ff##line = ::vkp::GetBaseName(__FILE__); \
        ::vkp::Profile::Timer timer##line( ::vkp::Profile::Record(name, ff##line) )
    #define VKP_PSCOPE0(name, line) VKP_PSCOPE1(name, line)

    #define VKP_PSCOPE_NONE() VKP_PSCOPE0(__func__, __LINE__)
    #define VKP_PSCOPE_DESC(desc) VKP_PSCOPE0(desc, __LINE__)

    #define VKP_EXPAND_PSCOPE(_0,_1,fname, ...) fname

// -----------------------------
    #define VKP_PROFILE_SCOPE(...) \
        VKP_EXPAND_PSCOPE(_0, ##__VA_ARGS__, VKP_PSCOPE_DESC, VKP_PSCOPE_NONE)(__VA_ARGS__)
// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^
#else
    #define VKP_PROFILE_SCOPE(...)
#endif


namespace vkp
{
    /**
     * @brief Singleton for global profiling:
     *  a) Keeps a global data structure of profiling records.
     *  b) Profiling records can be inserted into the data structure from
     *      anywhere using compile-time macros:
     *      foo():
     *      {
     *          VKP_PROFILE_SCOPE() // (1)
     *          ...
     *      }   // (2) The time taken 't' from (1) to here is recorded.
     *
     *  Generates profile record at (2):
     *      { name: "foo", duration: t, fileName: __FILE__ }
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
            const char* fileName;

            Record(const char* str, const char* filename)
                : Record(str, 0.0f, filename) {}
            
            Record(const char* str, float val, const char* filename)
                : name(str), duration(val), fileName(filename) {}

            bool operator==(const Record& o) const
            {
                return o.name == name;
            }
        };

        /**
         * @return All records ordered from most recently inserted
         */
        static std::vector<Record> GetRecordsFromLatest();

        /**
         * @brief Inserts a record into the global profile data structure
         */
        static void InsertRecord(const Record& r);

        class Timer 
        {
        public:
            /**
             * @param record Profile record that is going to be inserted at the
             *  end of timer's lifetime.
             */
            Timer(const Record& record)
                : m_Record(record), m_Stopped(false) { Start(); }

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
                m_Record.duration = ElapsedMillis();
                Profile::InsertRecord(m_Record);
            }

            inline float ElapsedMillis() const
            {
                return std::chrono::duration_cast<std::chrono::microseconds>
                    (std::chrono::high_resolution_clock::now() - m_Start).count() *
                    MICRO_TO_MILLIS;
            }

        private:
            Profile::Record m_Record;
            bool m_Stopped;

            std::chrono::time_point<std::chrono::high_resolution_clock> m_Start;
        };

    private:
        struct InternalRecord
        {
            const char* name;
            float duration;
            const char* fileName;

            InternalRecord* prev;
            InternalRecord* next;

            InternalRecord() : InternalRecord("Undef", 0.0f, nullptr) {}

            InternalRecord(const Record& record)
                : InternalRecord(record.name, record.duration, record.fileName) {}

            InternalRecord(const char* desc, float val, const char* filename)
                : name(desc), duration(val), fileName(filename),
                  prev{nullptr}, next{nullptr} {}
            
            bool operator==(const InternalRecord& o) const
            {
                return o.name == name &&
                       o.fileName == fileName;
            }
        };

        static inline std::unordered_map<const char*, InternalRecord> s_Records;
        static inline InternalRecord* s_LatestRecord{ nullptr };
    };

    /**
     * @return Base file name from a file path string literal,
     *  gets expanded at compile-time (even without temporary constexpr
     *  variable when optimzation is enabled).
     *  Tested with x86-64 versions of: gcc 12.2, clang 15.0, MSVC 19.0
     * TODO if not working then hash table of paths -> base file names
     */
    constexpr const char* GetBaseName(const char* path)
    {
        const char* p = path;
        while (*path != '\0')
        {
            if (*path == '/' || *path == '\\')  // Unix, Windows delim
                p = path;

            ++path;
        }
        return ++p;
    }
    
} // namespace vkp


#endif // WATER_SURFACE_RENDERING_CORE_PROFILE_H_
