
#ifndef NLogger_hpp
#define NLogger_hpp

#include <stdio.h>
#include <iostream>
#include <type_traits>
#include <vector>


// #include "fmt/format.h"
#include "fmt/bundled/format.h"
#include "fmt/bundled/ostream.h"
// #include "spdlog/fmt/bin_to_hex.h"

#define CHECK_FMT_STRING 1
#ifdef CHECK_FMT_STRING
#define FMT_STR(x) FMT_STRING(x)
#else
#define FMT_STR(x) x
#endif

#ifdef _WIN32
#define dbgt(OBJ, FMT, ...)     (OBJ)->trace(FMT_STR(FMT), __VA_ARGS__)
#define dbgd(OBJ, FMT, ...)     (OBJ)->debug(FMT_STR(FMT), __VA_ARGS__)
#define dbgi(OBJ, FMT, ...)     (OBJ)->info (FMT_STR(FMT), __VA_ARGS__)
#define dbgw(OBJ, FMT, ...)     (OBJ)->warn (FMT_STR(FMT), __VA_ARGS__)
#define dbge(OBJ, FMT, ...)     (OBJ)->error(FMT_STR(FMT), __VA_ARGS__)
#define dbgl(OBJ, LV, FMT, ...) (OBJ)->log(LV, FMT_STR(FMT), __VA_ARGS__)
#else
#define dbgt(OBJ, FMT, ARGS...)     (OBJ)->trace(FMT_STR(FMT), ##ARGS)
#define dbgd(OBJ, FMT, ARGS...)     (OBJ)->debug(FMT_STR(FMT), ##ARGS)
#define dbgi(OBJ, FMT, ARGS...)     (OBJ)->info (FMT_STR(FMT), ##ARGS)
#define dbgw(OBJ, FMT, ARGS...)     (OBJ)->warn (FMT_STR(FMT), ##ARGS)
#define dbge(OBJ, FMT, ARGS...)     (OBJ)->error(FMT_STR(FMT), ##ARGS)
#define dbgl(OBJ, LV, FMT, ARGS...) (OBJ)->log(LV, FMT_STR(FMT), ##ARGS)
#endif

class NObjDumper{
#define EQUSTR  "="
#define OBJBSTR  "{{"
#define OBJESTR  "}}"
    
    static const char kDIV = ',';
    static const int  kIndent = 2;
    
public:
    class Dumpable{
    public:
        virtual NObjDumper& dump(NObjDumper&& dumper) const {
            return dump(dumper);
        }
        
        virtual NObjDumper& dump(NObjDumper& dumper) const = 0;
    };
    
public:
    inline NObjDumper(const std::string prefix)
    :prefix_(prefix){
        
    }
    
    virtual ~NObjDumper(){
        
    }
    
    inline void checkChar(char c){
        if(c) fmt::format_to(mbuffer_, "{}", c);
    }
    
    template <class T>
    inline NObjDumper& kv(const std::string& name, const T &value) {
        checkIndent();
        checkChar(div_);
        fmt::format_to(mbuffer_, "{}" EQUSTR "{}", name, value);
        div_ = kDIV;
        return *this;
    }
    
    template <class T>
    inline NObjDumper& dump(const std::string& name, const T &value){
        return dump(name, value, kIndent);
    }
    
    template <class T>
    inline NObjDumper& dump(const std::string& name, const T &value, int newlint_indent){
        checkIndent();
        checkChar(div_);
        fmt::format_to(mbuffer_, "{}" EQUSTR, name);
        div_ = '\0';
        newlineIndent_ +=newlint_indent;
        auto& o = value.dump(*this);
        newlineIndent_ -=newlint_indent;
        o.div_ = kDIV;
        return o;
    }
    
    template <class T>
    inline NObjDumper& dump(const std::string& name, std::vector<T> v){
        checkIndent();
        checkChar(div_);
        fmt::format_to(mbuffer_, "{}" EQUSTR OBJBSTR, name);
        div_ = '\0';
        NObjDumper& dumper = *this;
        if(v.size() > 0){
            newlineIndent_ +=kIndent;
            endl();
            dump(v);
            newlineIndent_ -=kIndent;
        }
        //os_ << kOBJE;
        fmt::format_to(mbuffer_, OBJESTR);
        div_ = kDIV;
        return dumper;
    }
    
    template <class T>
    inline NObjDumper& dump(std::vector<T> v){
        NObjDumper& dumper = *this;
        int n = 0;
        for(auto& o : v){
            ++n;
            char str[64];
            sprintf(str, "No[%d/%zu]", n, v.size());
            dumper.dump(str, o).endl();
        }
        return dumper;
    }
    
    template <class T>
    NObjDumper& dump(const T &value){
        checkIndent();
        return value.dump(*this);
    }
    
    
    template <class T>
    NObjDumper& v(const T &value){
        checkIndent();
        fmt::format_to(mbuffer_, "{}", value);
        return *this;
    }
    
    inline NObjDumper& objB(){
        checkIndent();
        fmt::format_to(mbuffer_, OBJBSTR);
        return *this;
    }
    
    inline NObjDumper& objE(){
        checkIndent();
        fmt::format_to(mbuffer_, OBJESTR);
        return *this;
    }
    
    virtual void onEndl() = 0;
    
    inline NObjDumper& endl(){
        checkChar(div_);
        static char c = '\0';
        mbuffer_.append(&c, (&c)+1);
        onEndl();
        mbuffer_.clear();
        
        div_ = '\0';
        indentPos_ = 0;
        prefixPos_ = 0;
        if(newlineIndent_ > 0){
            indent_ += newlineIndent_;
            checkIndent();
            indent_ -= newlineIndent_;
        }
        return *this;
    }
    
    NObjDumper& indent(int n = kIndent){
        indent_ += n;
        return *this;
    }
    
    NObjDumper& uindent(int n = kIndent){
        indent_ -= n;
        return *this;
    }
    
    inline NObjDumper& checkIndent(){
        if(prefixPos_ < prefix_.length()){
            fmt::format_to(mbuffer_, "{}", prefix_);
            prefixPos_ = prefix_.length();
        }
        
        for(; indentPos_ < indent_; ++indentPos_){
            fmt::format_to(mbuffer_, " ");
        }
        return *this;
    }
    
protected:
    std::string prefix_;
    size_t prefixPos_ = 0;
    //std::ostream& os_;
    char div_ = '\0';
    int indent_ = 0;
    int indentPos_ = 0;
    int newlineIndent_ = 0;
    fmt::memory_buffer mbuffer_;
};

class NObjDumperOStream : public NObjDumper{
public:
    inline NObjDumperOStream(std::ostream& os, const std::string prefix = "")
    :NObjDumper(prefix), os_(os){
        
    }
    
    virtual ~NObjDumperOStream(){
        
    }
    
    virtual inline void onEndl() override{
        os_ << mbuffer_.data() << std::endl;
    }
    
protected:
    std::ostream& os_;
};



class NLogger{
private:
    using ThizClass = NLogger;
public:
    typedef std::shared_ptr<ThizClass> shared;
    
    struct Level0{
        enum Type
        {
            trace = 0,
            debug ,
            info ,
            warn ,
            err ,
            critical ,
            off ,
        };
        
        static char ShortName(Type typ){
            static char names[]={ 'T', 'D', 'I', 'W', 'E', 'C' };
            return names[typ];
        }
    };
    using Level = Level0::Type;
    
    
    static shared Get(const std::string& name){
        shared logger =  std::make_shared<ThizClass>(name);
        return logger;
    }
    
    
    static void EnableSinks(const std::string& app,
                     bool log_to_console,
                     const std::string& log_path);
    
    static void EnableSinks(const std::string& base_filename,
                     bool log_to_console);
    
    static void Exit();
    
    static void setEnv(Level level, bool flush);
    
public:
    NLogger(){}
    NLogger(const std::string& name):name_(name){}
    
    shared child(const std::string& sub_name){
        shared logger =  std::make_shared<ThizClass>(childName(sub_name));
        return logger;
    }
    
    std::string childName(const std::string& sub_name){
        return name_ + "|" + sub_name;
    }
    
    template <typename S, typename... Args
    //, class = typename std::enable_if<fmt::is_compile_string<S>::value>::type
    >
    void trace(const S &format_str, const Args &... args);

    template <typename S, typename... Args
    //, class = typename std::enable_if<fmt::is_compile_string<S>::value>::type
    >
    void debug(const S &format_str, const Args &... args);

    template <typename S, typename... Args
    //, class = typename std::enable_if<fmt::is_compile_string<S>::value>::type
    >
    void info(const S &format_str, const Args &... args);

    template <typename S, typename... Args
    //, class = typename std::enable_if<fmt::is_compile_string<S>::value>::type
    >
    void warn(const S &format_str, const Args &... args);

    template <typename S, typename... Args
    //, class = typename std::enable_if<fmt::is_compile_string<S>::value>::type
    >
    void error(const S &format_str, const Args &... args);

    template <typename S, typename... Args
    //, class = typename std::enable_if<fmt::is_compile_string<S>::value>::type
    >
    void critical(const S &format_str, const Args &... args);
    
    
    template <typename S, typename... Args
    //, class = typename std::enable_if<fmt::is_compile_string<S>::value>::type
    >
    void log(Level level, const S &fmt, const Args &... args);
    
    void indent(int indent=2){
        indent_ += indent;
    }
    
    void unindent(int indent=-2){
        indent_ += indent;
    }
    
    shared indentClone(int indent=2){
        shared logger =  std::make_shared<ThizClass>(name_);
        logger->indent(indent);
        return logger;
    }
    
    void flush(){
        //env().root->flush();
    }
    
private:
    void append_time(fmt::memory_buffer& mbuf);
    
    void output(const Level level, fmt::memory_buffer& mbuf);

private:
    const std::string name_;
    int indent_ = 0;
    
private:
    
    static void updateEnv(){

    }
    
//    struct Env{
//        std::shared_ptr<spdlog::logger> root ;
//        Level level = Level::debug;
//        bool flush = true;
//    };
//
//    static inline
//    Env& env(){
//        static Env env;
//        if(!env.root){
//            env.root = spdlog::stdout_logger_mt("root");
//            updateEnv();
//        }
//        return env;
//    }
};

template <typename S, typename... Args>
inline void NLogger::trace(const S& fmt, const Args &... args)
{
    log(Level::trace, fmt, args...);
}

template <typename S, typename... Args>
inline void NLogger::debug(const S& fmt, const Args &... args)
{
    log(Level::debug, fmt, args...);
}

template <typename S, typename... Args>
inline void NLogger::info(const S& fmt, const Args &... args)
{
    log(Level::info, fmt, args...);
}

template <typename S, typename... Args>
inline void NLogger::warn(const S& fmt, const Args &... args)
{
    log(Level::warn, fmt, args...);
}

template <typename S, typename... Args>
inline void NLogger::error(const S& fmt, const Args &... args)
{
    log(Level::err, fmt, args...);
}

template <typename S, typename... Args>
inline void NLogger::critical(const S& fmt, const Args &... args)
{
    log(Level::critical, fmt, args...);
}

template <typename S, typename... Args>
inline void NLogger::log(Level level, const S &fmt, const Args &... args){
    fmt::memory_buffer mbuffer;
    
    mbuffer.push_back('|');
    append_time(mbuffer);
    mbuffer.push_back('|');
    mbuffer.push_back(Level0::ShortName(level));
    mbuffer.push_back('|');
    
    //fmt::format_to(mbuffer, "{}| ", name_);
    mbuffer.append(name_.data(), name_.data()+name_.size());
    mbuffer.push_back('|');
    mbuffer.push_back(' ');
    
    if(indent_ > 0){
        for(int i = 0; i < indent_; ++i){
            mbuffer.push_back(' ');
        }
    }
    
    fmt::format_to(mbuffer, fmt, args...);
    mbuffer.push_back('\0');
    output(level, mbuffer);
    
//    env().root->log(level, mbuffer.data());
//
//    if(env().flush){
//        flush();
//    }
}

class NObjDumperLog : public NObjDumper{
public:
    inline NObjDumperLog(NLogger &logger
                         , NLogger::Level level = NLogger::Level::info
                         , const std::string prefix = "")
    :NObjDumper(prefix), logger_(logger), level_(level){
        
    }
    
    virtual ~NObjDumperLog(){
        if(mbuffer_.size() > 0){
            endl();
        }
    }
    
    virtual inline void onEndl() override{
        // TODO: optimize
        logger_.log(level_, fmt::format("{}"), mbuffer_.data());
    }
    
protected:
    NLogger &logger_;
    NLogger::Level level_;
};

#endif /* NLogger_hpp */