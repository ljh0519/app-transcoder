
#ifndef NStr_hpp
#define NStr_hpp

#include <string>

namespace NStr{
    
    inline
    bool StartWith(const std::string& str, const std::string& prefix) {
        return prefix.length() <= str.length()
        && std::equal(prefix.begin(), prefix.end(), str.begin());
    }
    
    inline
    bool StartWith(const std::string& str, size_t pos, const std::string& prefix) {
        if(pos > str.size()){
            return false;
        }
        
        size_t size = str.size() - pos;
        return (prefix.length() <= size
                && std::equal(prefix.begin(), prefix.end(), str.begin()+pos));
    }
    
    inline
    bool EndWith(const std::string& str, const std::string& postfix){
        return postfix.length() <= str.length()
        && std::equal(postfix.begin(), postfix.end(), str.end()-postfix.length());
    }
    
    inline
    std::string RemovePrefix(const std::string& str, const char c){
        for(size_t i = 0; i < str.size(); ++i){
            if(str[i] != c){
                return str.substr(i, str.size()-i);
            }
        }
        return std::string("");
    }
    
    inline
    std::string RemovePostfix(const std::string& str, const char c){
        for(size_t i = 0; i < str.size(); ++i){
            if(str[str.size()-i-1] != c){
                return str.substr(0, str.size()-i);
            }
        }
        return std::string("");
    }
    
    // for string delimiter
    inline
    std::vector<std::string> Split (std::string s, std::string delimiter) {
        size_t pos_start = 0, pos_end, delim_len = delimiter.length();
        std::string token;
        std::vector<std::string> res;
        
        while ((pos_end = s.find (delimiter, pos_start)) != std::string::npos) {
            token = s.substr (pos_start, pos_end - pos_start);
            pos_start = pos_end + delim_len;
            res.push_back (token);
        }
        
        res.push_back (s.substr (pos_start));
        return res;
    }
    
};

#endif // NStr_hpp
