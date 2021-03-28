#include<list>
#include<regex>
#include<string>
#include<fstream>
#include<iostream>
#include<unordered_set>
#include<unordered_map>

std::unordered_set<std::string> keyword_set = {"if","then","else","end","repeat","until","read","write"};
std::unordered_set<std::string> signal_set = {"+","++","+=","-","--","-=","*","**","*=","/","/=","%","%="
                                                ,"=",":=","!=","==","<=",">=","<",">","(",")","{","}",";"};

namespace str{
    bool is_number(std::string &s){
        return std::regex_match(s,std::regex("^[0-9]+$"));
    }
    bool is_signal(std::string &s){
        return  signal_set.find(s) != signal_set.end();
    }
    bool is_keyword(std::string &s){
        return keyword_set.find(s) != keyword_set.end();
    }
    bool is_identifier(std::string &s){
        return std::regex_match(s,std::regex("^[a-zA-Z_]{1}\\w*$")) && !is_keyword(s);
    }
    bool noting(std::string &s){
        return *s.begin() == '{' && *(s.end()-1) == '}';
    }
    bool is_semicolon(std::string &s){
        return s==";";
    }
}

// id & num = id
// id & key = key
// id & num & key = key
// sig & other = invalid
// blank & other = invalid
// 共5位，sig, blank 低2位， id,key,number 高3位
// 第6位用来表示空白
enum class token_t{
    init = 0b11111, signal = 0b00001, keyword = 0b10000, identifier = 0b11000, number = 0b11100, invalid = 0, blank = 0b00010
};
token_t get_token_t(std::string &s, token_t &probably_t){
    using namespace str;
    switch (probably_t){
        case token_t::init:
            return token_t::init;
        case token_t::signal:
            return is_signal(s) ? token_t::signal : token_t::invalid;
        case token_t::keyword:
        case token_t::identifier:
            token_t t;
            if(is_keyword(s))
                t = token_t::keyword;
            else if(is_identifier(s))
                t = token_t::identifier;
            else
                t = token_t::invalid;
            return t;
        case token_t::number:
            return is_number(s) ? token_t::number : token_t::invalid;
        case token_t::invalid:
            return token_t::invalid;
    }
    return token_t::invalid;
}
enum class char_t{
    digit, alphabet, signal, space, line_end, invalid
};
char_t get_char_t(char &c){
    if (c>='0' && c<='9')
        return char_t::digit;
    else if((c>='a' && c<='z' ) || (c>='A' && c<='Z' ))
        return char_t::alphabet;
    else if(c == '{' || c == '}' || c=='+' || c=='-' || c=='*' || c=='/' || c=='%' || c=='=' || c=='<' || c=='>' || c=='(' || c==')' || c==';' || c==':')
        return char_t::signal;
    else if(c==' ')
        return char_t::space;
    else if(c=='\r' || c=='\n')
        return char_t::line_end;
    else
        return char_t::invalid;
}

class lexer{
void update_type(token_t &tt,token_t &ltt, char_t &ct, char_t &lct){
    ltt = tt;
    lct = ct;
}
public:
    std::unordered_map<token_t, std::string> token_str;
    std::unordered_map<char_t, std::string> char_str;

    lexer(){
        token_str[token_t::identifier] = "identifier";
        token_str[token_t::invalid] = "invalid";
        token_str[token_t::keyword] = "keyword";
        token_str[token_t::number] = "number";
        token_str[token_t::signal] = "signal";
        token_str[token_t::init] = "init";
        char_str[char_t::space] = "space";
        char_str[char_t::digit] = "digit";
        char_str[char_t::signal] = "signal";
        char_str[char_t::invalid] = "invalid";
        char_str[char_t::line_end] = "line_end";
        char_str[char_t::alphabet] = "alphabet";
    }

    struct token{
        token_t _t;
        int row,col;
        std::string val;
        token(token_t _t, std::string &val, int &row, int &col):
        _t(_t), val(val), row(row), col(col){};
    };
    std::list<token> token_stream;

    void wirte_token_stream(std::ofstream &ofs){
        for(auto it : token_stream)
            ofs<<it.row<<","<<it.col<<": "<<token_str[it._t]<<" "<<it.val<<std::endl;
    }

    void analyse(std::ifstream &ifs){
        std::ofstream ofs("info");
        char c;
        token_t t_t = token_t::init, last_t_t = token_t::init, current_t_t = token_t::init;
        char_t c_t = char_t::invalid, last_c_t = char_t::invalid;
        std::string buffer;
        bool noting = false;
        int row = 1, col = 0;

        while (ifs.get(c)){

            if(buffer.empty())
                t_t = token_t::init;
            c_t = get_char_t(c);
            col++;

            if(c_t==char_t::line_end && last_c_t!=char_t::line_end) // 换行
                row++, col = 0;

            if(noting && c!='}') { // 注释
                update_type(t_t,last_t_t,c_t,last_c_t);
                continue;
            }

            switch(c_t){
                case char_t::digit:
                    current_t_t = token_t::number;
                    t_t = token_t((int)t_t&(int)token_t::number);
                break;
                case char_t::alphabet:
                    current_t_t = token_t::identifier;
                    t_t = token_t((int)t_t&(int)token_t::identifier);
                break;
                case char_t::signal:
                    current_t_t = token_t::signal;
                    t_t = token_t((int)t_t&(int)token_t::signal);
                    if(c=='{') {
                        update_type(t_t,last_t_t,c_t,last_c_t);
                        noting = true;
                        continue;
                    }
                    if(c=='}') {
                        noting = false;
                        update_type(t_t,last_t_t,c_t,last_c_t);
                        continue;
                    }
                break;
                case char_t::space:
                    current_t_t = token_t::blank;
                    t_t = token_t((int)t_t&(int)token_t::blank);
                break;
                case char_t::line_end:
                    current_t_t = token_t::blank;
                    t_t = token_t((int)t_t&(int)token_t::blank);
                break;
                case char_t::invalid:
                    current_t_t = token_t::invalid;
                    t_t = token_t::invalid;
                break;
            }

            if(!noting) { // 正文处理tokens
                if (t_t == token_t::invalid && !buffer.empty()) {
                    token_t tmp = get_token_t(buffer, last_t_t);
                    if(tmp!=token_t::invalid && tmp!=token_t::init && tmp!=token_t::blank)
                        token_stream.push_back(token(tmp, buffer, row, col));
                    buffer.clear();
                    t_t = current_t_t;
                }

                buffer.push_back(c);
            }

            // 无论如何处理字符，不论注释
            update_type(t_t,last_t_t,c_t,last_c_t);

            ofs<< "char: " << c << "\t|| chartype: " << char_str[c_t] << "\t|| lastchartype: " << char_str[last_c_t] << "\t|| tokentype: " << token_str[t_t] << "\t|| lasttokentype: " << token_str[last_t_t] <<std::endl;

        }
    }
};
