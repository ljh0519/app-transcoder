
#include "NLogger.hpp"
#include "NStr.hpp"

// #if defined(__linux__) || defined(__APPLE__)
// #include "g3log/g3log.hpp"
// #include "g3log/logworker.hpp"
// #else 
// #define LOG2STD 1
// #endif

#define LOG2STD 1


#include <iostream>
#include <chrono>
#ifdef _WIN32
#include <ctime>
#endif

namespace fmthelper{
    template<typename T>
    inline void append_int(T n, fmt::memory_buffer &dest)
    {
        fmt::format_int i(n);
        dest.append(i.data(), i.data() + i.size());
    }
    
    inline void pad2(int n, fmt::memory_buffer &dest)
    {
        if (n > 99)
        {
            append_int(n, dest);
        }
        else if (n > 9) // 10-99
        {
            dest.push_back(static_cast<char>('0' + n / 10));
            dest.push_back(static_cast<char>('0' + n % 10));
        }
        else if (n >= 0) // 0-9
        {
            dest.push_back('0');
            dest.push_back(static_cast<char>('0' + n));
        }
        else // negatives (unlikely, but just in case, let fmt deal with it)
        {
            fmt::format_to(dest, "{:02}", n);
        }
    }
    
    inline void pad3(int n, fmt::memory_buffer &dest){
        if(n < 10){
            dest.push_back('0');
            dest.push_back('0');
        }else if(n < 100){
            dest.push_back('0');
        }
        append_int(n, dest);
    }
}

namespace logfmt {
    //typedef std::chrono::high_resolution_clock clock;
    typedef std::chrono::system_clock clock;
    typedef std::chrono::time_point<clock> time_point;
    
    
    // from spdlog
    
    inline std::tm localtime(const std::time_t &time_tt) //SPDLOG_NOEXCEPT
    {
        
#ifdef _WIN32
        std::tm tm;
        localtime_s(&tm, &time_tt);
#else
        std::tm tm;
        localtime_r(&time_tt, &tm);
#endif
        return tm;
    }
    
    template<typename ToDuration>
    static inline ToDuration time_fraction(const time_point &tp)
    {
        using std::chrono::duration_cast;
        using std::chrono::seconds;
        auto duration = tp.time_since_epoch();
        auto secs = duration_cast<seconds>(duration);
        return duration_cast<ToDuration>(duration) - duration_cast<ToDuration>(secs);
    }
    
    
    std::tm get_time_(time_point tp){
        return localtime(clock::to_time_t(tp));
    }
    
    
    
    inline void append_time(fmt::memory_buffer& mbuf){
        // |20-02-03 12:55:02.589|I|mcu-1|cc|
        time_point tp = clock::now();
        
        {
            //static const char *zeroes = "0000000000000000000";
            static std::tm cached_tm_={0};
            static std::chrono::seconds last_log_secs_(0);
            //std::memset(&cached_tm_, 0, sizeof(cached_tm_));
            
            auto secs = std::chrono::duration_cast<std::chrono::seconds>(tp.time_since_epoch());
            if (secs != last_log_secs_){
                cached_tm_ = get_time_(tp);
                last_log_secs_ = secs;
            }
            
            // year
            //helper::append_int(cached_tm_.tm_year + 1900, mbuf);
            //mbuf.push_back('-');
            
            // month
            fmthelper::pad2(cached_tm_.tm_mon + 1, mbuf);
            mbuf.push_back('-');
            
            // day of month 1-31
            fmthelper::pad2(cached_tm_.tm_mday, mbuf);
            mbuf.push_back(' ');
            
            // hours in 24 format 0-23
            fmthelper::pad2(cached_tm_.tm_hour, mbuf);
            mbuf.push_back(':');
            
            // minutes 0-59
            fmthelper::pad2(cached_tm_.tm_min, mbuf);
            mbuf.push_back(':');
            
            // seconds 0-59
            fmthelper::pad2(cached_tm_.tm_sec, mbuf);
            mbuf.push_back('.');
        }
        
        
        { // milliseconds fraction 0-999
            auto millis = time_fraction<std::chrono::milliseconds>(tp);
            uint32_t frac_ms = static_cast<uint32_t>(millis.count());
            fmthelper::pad3(frac_ms, mbuf);
        }
    }
}

struct NLoggerWorker{

#ifndef LOG2STD
    static std::string G3CustomLogToString(const g3::LogMessage& msg){
        return "";
//        std::string out;
//        out.append(msg.timestamp() + "\t"
//                   + msg.level()
//                   + " ["
//                   + msg.threadID()
//                   + " "
//                   + msg.file()
//                   + "->"+ msg.function()
//                   + ":" + msg.line() + "]\t");
//        return out;
    }
    
    void initG3(const std::string& app,
                const std::string& log_path){
        using namespace g3;
        
        g3worker_ = LogWorker::createLogWorker();
        g3fsink_ = g3worker_->addSink(std::make_unique<FileSink>(app, log_path, ""),
                                      &FileSink::fileWrite);
        
        initializeLogging(g3worker_.get());
        g3fsink_->call(&FileSink::overrideLogDetails, &G3CustomLogToString);
        g3fsink_->call(&FileSink::overrideLogHeader, "\t\t-------------\n");
        
        //std::future<std::string> log_file_name = g3fsink_->call(&FileSink::fileName);
        //std::cout << "* Log file: [" << log_file_name.get() << "]\n\n" << std::endl;
        //LOG(INFO) << "one: " << 1;
        
    }
#endif
    
    void initWith(const std::string& app,
                  bool log_to_console,
                  const std::string& log_path){
        if(init_){
            return;
        }
        init_ = true;
        
        log_to_console_ = log_to_console;
        
        if(!log_path.empty()){
#ifndef LOG2STD
            initG3(app, log_path);
#endif
        }
    }
    
    void exit(){
#ifndef LOG2STD
        g3fsink_.reset();
        g3worker_.reset();
#endif
    }

    void setLogLevel(const NLogger::Level level) {
        if(level >= NLogger::Level::trace && level <= NLogger::Level::off) {
            defaultLevel_ = level;
        }
    }

    
    bool init_ = false;
    bool log_to_console_ = true;
#ifndef LOG2STD
    std::unique_ptr<g3::LogWorker> g3worker_;
    std::unique_ptr<g3::SinkHandle<g3::FileSink>> g3fsink_;
#endif
    NLogger::Level defaultLevel_ = NLogger::Level::debug;
}; // NLoggerWorker


static NLoggerWorker worker;
    

void NLogger::EnableSinks(const std::string& app,
                   bool log_to_console,
                   const std::string& log_path){
    worker.initWith(app, log_to_console, log_path);
}

void NLogger::EnableSinks(const std::string& base_filename,
                   bool log_to_console){
    std::string log_path;
    std::string app;

    std::size_t found = base_filename.find_last_of("/\\");
    if(std::string::npos != found) {
        log_path = base_filename.substr(0,found);
        app = base_filename.substr(found+1);
    }

    worker.initWith(app, log_to_console, log_path);
}

void NLogger::Exit(){
    worker.exit();
}

void NLogger::setEnv(Level level, bool flush){
    worker.setLogLevel(level);
}

void NLogger::append_time(fmt::memory_buffer& mbuf){
    logfmt::append_time(mbuf);
}

void NLogger::output(const Level level, fmt::memory_buffer& mbuf){
    if(worker.defaultLevel_ > level) {
        return ;
    }

    if(worker.log_to_console_){
        std::cout << mbuf.data() << std::endl;
        //printf("%.*s\n", (int)mbuf.size(), mbuf.data());
    }
    
#ifndef LOG2STD
    if(worker.g3fsink_){
        LOG(INFO) << mbuf.data();
    }
#endif
}