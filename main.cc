/**********************************************************************
   main.cc by kql2 (ltqsoft@gmail.com)
 Note: The program will try to use Cannadic as primary reference.

MIT License

Copyright (c) 2021 流陳光

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
**********************************************************************/

#include "strdef.hh"
#include <fstream>
#include <functional>
#include <iconv.h>
#include <iostream>
#include <list>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
//#include <utility>

#define BOM                     0xffffffef
#define CANNA_TEST_FILE         "../tests/kasumi-hinsi_codes-37.u8"
#define MS_TEST_FILE_UTF16LE    "../tests/w10-ime-tango-kq0.606.txt"
#define MS_TEST_FILE_UTF8_CRLF  "../tests/w10-ime-tango-kq0.606-utf8-crlf.txt"
#define MS_TEST_FILE_UTF8_LF    "../tests/w10-ime-tango-kq0.606-utf8-lf.txt"

#define cnm_skip(condition)     if(condition) continue
#define cnm_throw(test, msg)    if(test) throw std::runtime_error(msg)

#define FORMAT_CANNA  true
#define FORMAT_MSIME  false


constexpr const char* const HELP_MSG = R"(canmic [option] file
OPTIONS:
    -c --canna:         Read <file> as Cannadic format.
    -m --msime:         Read <file> as Microsoft IME format.
    -o --out=filename   Specify output file. If omitted, output
                        file name shall be "<file>_out.canmic"
)";


struct Opt
{
    static const std::string    STR_READ_CANNA_SHORT,
                                STR_READ_CANNA_LONG,
                                STR_READ_MSIME_SHORT,
                                STR_READ_MSIME_LONG,
                                STR_OUTPUT_FILENAME_SHORT,
                                STR_OUTPUT_FILENAME_LONG
    ;
};

/*static*/ const std::string Opt::STR_READ_CANNA_LONG        = "--canna";
/*static*/ const std::string Opt::STR_READ_CANNA_SHORT       = "-c";
/*static*/ const std::string Opt::STR_READ_MSIME_LONG        = "--msime";
/*static*/ const std::string Opt::STR_READ_MSIME_SHORT       = "-m";
/*static*/ const std::string Opt::STR_OUTPUT_FILENAME_LONG   = "--output=";
/*static*/ const std::string Opt::STR_OUTPUT_FILENAME_SHORT  = "-o";


enum class Hinshi {
    FUKASHI,            // 付加詞
    FUKUSHI,            // 副詞
    FUKUSHI_SURU,       // する副詞
    FUKUSHI_TO,         // と副詞
    FUKUSHI_TO_SURU,    // とする副詞
    FUKUSHI_TO_TARU,    // とたる副詞
    JIDOUSHI_1,         // "一段動詞"
    JIDOUSHI_BA5,       // ば五動詞
    JIDOUSHI_GA5,       // が五動詞
    JIDOUSHI_KA5,       // か五動詞
    JIDOUSHI_MA5,       // ま五動詞
    JIDOUSHI_NA5,       // な五動詞
    JIDOUSHI_RA5,       // ら五動詞
    JIDOUSHI_SA5,       // さ五動詞
    JIDOUSHI_TA5,       // た五動詞
    JIDOUSHI_WA5,       // わ五動詞
    KANJI_TAN,          // 単漢字
    KANDOUSHI,          // 間投詞、感動詞
    KEIYOUSHI,          // 形容詞
    MEI_CHI,            // 地名
    MEI_JIN,            // 人名
    MEI_KAISHA,         // 会社名
    MEISHI,             // 名詞
    MEISHI_NA,          // な名詞
    MEISHI_NA_SA,       // なさ名詞
    MEISHI_NA_SA_SURU,  // なさする名詞
    MEISHI_NA_SURU,     // なする名詞
    MEISHI_SURU,        // する名詞
    SUUJI,              // 数字
    TADOUSHI_BA5,       // ば五他動詞
    TADOUSHI_GA5,       // が五他動詞
    TADOUSHI_KA5,       // か五他動詞
    TADOUSHI_MA5,       // ま五他動詞
    TADOUSHI_NA5,       // な五他動詞
    TADOUSHI_RA5,       // ら五他動詞
    TADOUSHI_SA5,       // さ五他動詞
    TADOUSHI_TA5,       // た五他動詞
    TADOUSHI_WA5,       // わ五他動詞
    UNKNOWN             // unknown
};


const std::map<Hinshi, std::string> HINSHI_NAMES = {
    {Hinshi::FUKASHI,           u8"付加詞"},
    {Hinshi::FUKUSHI,           u8"副詞"},
    {Hinshi::FUKUSHI_SURU,      u8"する副詞"},
    {Hinshi::FUKUSHI_TO,        u8"と副詞"},
    {Hinshi::FUKUSHI_TO_SURU,   u8"とする副詞"},
    {Hinshi::FUKUSHI_TO_TARU,   u8"とたる副詞"},
    {Hinshi::JIDOUSHI_1,        u8"一段動詞"},
    {Hinshi::JIDOUSHI_BA5,      u8"ば五動詞"},
    {Hinshi::JIDOUSHI_GA5,      u8"が五動詞"},
    {Hinshi::JIDOUSHI_KA5,      u8"か五動詞"},
    {Hinshi::JIDOUSHI_MA5,      u8"ま五動詞"},
    {Hinshi::JIDOUSHI_NA5,      u8"な五動詞"},
    {Hinshi::JIDOUSHI_RA5,      u8"ら五動詞"},
    {Hinshi::JIDOUSHI_SA5,      u8"さ五動詞"},
    {Hinshi::JIDOUSHI_TA5,      u8"た五動詞"},
    {Hinshi::JIDOUSHI_WA5,      u8"わ五動詞"},
    {Hinshi::KANJI_TAN,         u8"単漢字"},
    {Hinshi::KANDOUSHI,         u8"感動詞"},
    {Hinshi::KEIYOUSHI,         u8"形容詞"},
    {Hinshi::MEI_CHI,           u8"地名"},
    {Hinshi::MEI_JIN,           u8"人名"},
    {Hinshi::MEI_KAISHA,        u8"会社名"},
    {Hinshi::MEISHI,            u8"名詞"},
    {Hinshi::MEISHI_NA,         u8"な名詞"},
    {Hinshi::MEISHI_NA_SA,      u8"なさ名詞"},
    {Hinshi::MEISHI_NA_SA_SURU, u8"なさする名詞"},
    {Hinshi::MEISHI_NA_SURU,    u8"なする名詞"},
    {Hinshi::MEISHI_SURU,       u8"する名詞"},
    {Hinshi::SUUJI,             u8"数字"},
    {Hinshi::TADOUSHI_BA5,      u8"ば五他動詞"},
    {Hinshi::TADOUSHI_GA5,      u8"が五他動詞"},
    {Hinshi::TADOUSHI_KA5,      u8"か五他動詞"},
    {Hinshi::TADOUSHI_MA5,      u8"ま五他動詞"},
    {Hinshi::TADOUSHI_NA5,      u8"な五他動詞"},
    {Hinshi::TADOUSHI_RA5,      u8"ら五他動詞"},
    {Hinshi::TADOUSHI_SA5,      u8"さ五他動詞"},
    {Hinshi::TADOUSHI_TA5,      u8"た五他動詞"},
    {Hinshi::TADOUSHI_WA5,      u8"わ五他動詞"},
    {Hinshi::UNKNOWN,           HINSHI_STR_UNKNOWN}
};


const std::map<std::string, Hinshi> HINSHI_TABLE_CANNA = {
    {"#K5r",    Hinshi::TADOUSHI_KA5},
    {"#K5",     Hinshi::JIDOUSHI_KA5},
    {"#CJ",     Hinshi::KANDOUSHI},
    {"#KK",     Hinshi::MEI_KAISHA},
    {"#G5r",    Hinshi::TADOUSHI_GA5},
    {"#G5",     Hinshi::JIDOUSHI_GA5},
    {"$KS",     Hinshi::JIDOUSHI_1},
    {"#KY",     Hinshi::KEIYOUSHI},
    {"#S5r",    Hinshi::TADOUSHI_SA5},
    {"#S5",     Hinshi::JIDOUSHI_SA5},
    {"#JN",     Hinshi::MEI_JIN},
    {"#NN",     Hinshi::SUUJI},
    {"#F12",    Hinshi::FUKUSHI_SURU},
    {"#T30",    Hinshi::MEISHI_SURU},
    {"#T5r",    Hinshi::TADOUSHI_TA5},
    {"#T5",     Hinshi::JIDOUSHI_TA5},
    {"#KJ",     Hinshi::KANJI_TAN},
    {"#CN",     Hinshi::MEI_CHI},
    {"#T35",    Hinshi::MEISHI},
    {"#F04",    Hinshi::FUKUSHI_TO_SURU},
    {"#F02",    Hinshi::FUKUSHI_TO_TARU},
    {"#F06",    Hinshi::FUKUSHI_TO},
    {"#N5r",    Hinshi::TADOUSHI_NA5},
    {"#N5",     Hinshi::JIDOUSHI_NA5},
    {"#T00",    Hinshi::MEISHI_NA_SA_SURU},
    {"#T05",    Hinshi::MEISHI_NA_SA},
    {"#T10",    Hinshi::MEISHI_NA_SURU},
    {"#T15",    Hinshi::MEISHI_NA},
    {"#B5r",    Hinshi::TADOUSHI_BA5},
    {"#B5",     Hinshi::JIDOUSHI_BA5},
    {"#RT",     Hinshi::FUKASHI},
    {"#F14",    Hinshi::FUKUSHI},
    {"#M5r",    Hinshi::TADOUSHI_MA5},
    {"#M5",     Hinshi::JIDOUSHI_MA5},
    {"#R5r",    Hinshi::TADOUSHI_RA5},
    {"#W5",     Hinshi::JIDOUSHI_WA5},
    {"#R5",     Hinshi::JIDOUSHI_RA5},
    {"#W5r",    Hinshi::TADOUSHI_WA5},
    {"#unk",    Hinshi::UNKNOWN}
};


const std::map<std::string, Hinshi> HINSHI_TABLE_MSIME = {
    { HINSHI_STR_MSIME_MEISHI, Hinshi::MEISHI },
    { HINSHI_STR_MSIME_MEI, Hinshi::MEI_JIN }, //$ lossy conversion: first name -> full name
    { HINSHI_STR_MSIME_MEI_JIN, Hinshi::MEI_JIN },
    { HINSHI_STR_MSIME_SEI, Hinshi::MEI_JIN }, //$ lossy conversion: last name -> full name
    { u8"地名その他", Hinshi::MEI_CHI },
    { u8"形容動詞", Hinshi::FUKUSHI_TO_TARU },
    { HINSHI_STR_MSIME_DOUSHI_NA5,  Hinshi::JIDOUSHI_NA5 },
    { HINSHI_STR_MSIME_DOUSHI_WA5,  Hinshi::JIDOUSHI_WA5 },
    { u8"か行五段", Hinshi::JIDOUSHI_KA5 },
    { u8"が行五段", Hinshi::JIDOUSHI_GA5 },
    { u8"ま行五段", Hinshi::JIDOUSHI_MA5 },
    { u8"ら行五段", Hinshi::JIDOUSHI_RA5 },
    { HINSHI_STR_MSIME_FUKUSHI, Hinshi::FUKUSHI },
    { u8"形容詞", Hinshi::KEIYOUSHI },
    { u8"短縮よみ", Hinshi::MEISHI }, //$ lossy conversion: abbr -> noun.
    { u8"形動名詞", Hinshi::MEISHI_NA }, //$ lossy conversion: keiyoudoushi -> na-noun
    { u8"接尾語", Hinshi::MEISHI }, //$ lossy conversion: suffix -> noun
    { u8"固有名詞", Hinshi::MEISHI }, //$ lossy conversion: private noun -> noun
    { u8"副詞的名詞", Hinshi::MEISHI }, //$ lossy: fukushiteki -> noun
    { u8"一段動詞", Hinshi::JIDOUSHI_1 },
    { "顔文字", Hinshi::SUUJI }, //$ lossy conversion: toumoji -> numbers
    { "さ変名詞", Hinshi::MEISHI_SURU },
    { "慣用句", Hinshi::MEISHI }, //$ lossy: idiom -> noun
    { HINSHI_STR_UNKNOWN, Hinshi::UNKNOWN }
};


struct Tango
{
    static constexpr uint16_t DEFAULT_FREQ = 500;
    enum class Format { CANNA, MSIME };

    static uint16_t num_converted; //$ to count entries converted.

    std::string value, furigana;
    std::string description;
    Hinshi hinshi;
    uint16_t frequency;

    bool is_valid() const;
};

typedef std::list<Tango> TangoContainer;


class Hinshi2String
{
private:
    class Base {
    public:
        Base(); virtual ~Base();
        const std::string& operator[](Hinshi wHinshi) const;
    protected:
        std::map<Hinshi, std::string> data_;
    };
public:
    class Canna final : public Base
    {
    public:
        Canna(); ~Canna();
    };

    class MSIME final : public Base
    {
    public:
        MSIME(); ~MSIME();
    };
};


inline void print(const Tango& t)
{
    std::cout << '[' << t.value <<
        "]  furigana=<" << t.furigana <<
        ">  hinshi=<" << HINSHI_NAMES.find(t.hinshi)->second <<
        ">  freq=" << t.frequency <<
        '\n';
}


inline void split_hinshi_and_frequency(
    const std::string& inStr, std::string& outHinshi, uint16_t& outFreq
){
    size_t asteriskPos = inStr.find('*');
    outHinshi = inStr.substr(0, asteriskPos);
    outFreq = std::stoul(inStr.substr(asteriskPos+1));
}


//$ Cannadic -> Microsoft IME (incomplete)
void read_canna(
    const char* const wFilePath, const std::string& wOutputFilename,
    TangoContainer& wOutput)
{
    std::cout << SEPARATOR << "Reading cannadic file: " <<
        wFilePath << '\n' << SEPARATOR;

    std::ifstream inputFile(wFilePath);
    std::ofstream outputFile(wOutputFilename, std::ios::trunc);
    cnm_throw(inputFile.fail(), "Cannot open input cannadic file!");
    cnm_throw(!outputFile.is_open(), "Cannot access output file!");

    std::string line;
    const Hinshi2String::MSIME h2s;
    while(std::getline(inputFile, line, '\n'))
    {
        std::istringstream iss(line);
        struct { std::string furigana, hinshiFreq, value; } str;
        std::string strHinshi;
        uint16_t frequency = Tango::DEFAULT_FREQ;

        iss >> str.furigana >> str.hinshiFreq >> str.value;
        split_hinshi_and_frequency(str.hinshiFreq, strHinshi, frequency);

        const Tango last = {
            str.value, str.furigana, std::string("<none>"),
            HINSHI_TABLE_CANNA.find(strHinshi)->second, frequency
        };

        outputFile <<
            last.furigana << '\t' << last.value << '\t' <<
            h2s[last.hinshi] <<
            "\r\n";
        ++Tango::num_converted;
    }

    inputFile.close();
    outputFile.close();

    //for(const auto& t : wOutput) { print(t); }
    std::cout << "Summary: " << Tango::num_converted <<
        " entry(ies) converted to file:" << wOutputFilename <<
        ".\n";
}


//$ Check if furigana contains the 'ヴ' character,
//$ since kasumi only support hiragana.
inline bool has_vu(const std::string& str)
{return(
    str.find("ヴ") != std::string::npos
);}


inline bool has_bom(const std::string& str)
{
    const uint16_t first3[] = {
        static_cast<unsigned short>(str[0]),
        static_cast<unsigned short>(str[1]),
        static_cast<unsigned short>(str[2])
    };
    return (
        ( (first3[0] & 0xff) == 0x00ef ) &&
        ( (first3[1] & 0xff) == 0x00bb ) &&
        ( (first3[2] & 0xff) == 0x00bf )
    );
}


//$ Microsoft IME -> Cannadic
void read_ms(
    const char* const wFilePath, const std::string& wOutputFilename,
    TangoContainer& wOutput)
{
    std::cout << SEPARATOR << "Reading Microsoft IME dicionary file: " <<
        wFilePath << '\n' << SEPARATOR;

    std::ifstream inputFile(wFilePath);
    std::ofstream outputFile(wOutputFilename, std::ios::trunc);
    if(inputFile.fail()) throw std::runtime_error("Cannot open Microsoft IME file!");
    if(!outputFile.is_open()) throw std::runtime_error("Cannot access output file!");
    std::string line;
    const Hinshi2String::Canna h2s;

    iconv_t instance = iconv_open("utf-8", "utf-16le");
    if(instance == (iconv_t)-1) {
        std::cerr << "ERROR: iconv initializaion FAILED!\n";
    }else{
        iconv_close(instance);
    }

    while(std::getline(inputFile, line, '\n'))
    {
        if(line[0] == '!' || has_bom(line)) {
            std::cout << "Skipped line: " << line << '\n';
            continue;
        } // no_else

        std::istringstream iss(line);
        struct {
            std::string furigana, value, hinshi_str, description;
        } str;

        std::getline(iss,str.furigana, '\t');
        std::getline(iss, str.value, '\t');
        std::getline(iss,str.hinshi_str, '\t');
        std::getline(iss, str.description, '\n');

        cnm_skip(str.furigana.empty());
        cnm_skip(has_vu(str.furigana));

        if(str.hinshi_str.back() == '\r') {
            str.hinshi_str.resize(str.hinshi_str.length()-1);
        } // no_else

        auto itr = HINSHI_TABLE_MSIME.find(str.hinshi_str);
        if(itr != HINSHI_TABLE_MSIME.end()) {
            Tango t = {
                str.value, str.furigana, str.description,
                itr->second, Tango::DEFAULT_FREQ
            };
            if(t.is_valid()) {
                outputFile << t.furigana << ' ' << h2s[t.hinshi] <<
                    '*' << t.frequency <<
                    ' ' << t.value << '\n';
                ++Tango::num_converted;
            }else{
                std::cerr << "WARNING: invalid tango detected!\n";
            }
        } else if(str.furigana[0] != '\r') {
            std::cerr << "WARNING: unknown hinshi of entry '" <<
                str.furigana << "'|'" << str.value << "'|'" << str.hinshi_str <<
                "'\n";
        } // no_else
    }

    std::cout << "Summary: " << Tango::num_converted <<
        " entry(ies) converted to file:" << wOutputFilename << ".\n";

    inputFile.close();
    outputFile.close();
}

///////////////////////////////////////////////////////////////

TangoContainer tangos;


int main(int argc, const char** argv)
{
    cnm_throw(
        HINSHI_NAMES.size() != HINSHI_TABLE_CANNA.size(),
        "Hinshi table incomplete!"
    );

    if(argc == 1) {
        std::cout << HELP_MSG;
        return 0;
    } // no_else

    std::function<
        void(const char* const, const std::string&, TangoContainer&)
    > convert = nullptr;

    std::string inputFilename, outputFilename_suffix, outputFilename;

    for(int i=1; i < argc; ++i)
    {
        // case: read input file as Cannadic dictionary.
        if(
            argv[i] == Opt::STR_READ_CANNA_SHORT ||
            argv[i] == Opt::STR_READ_CANNA_LONG
        ){
            convert = read_canna;
            outputFilename_suffix = OUTPUT_FILENAME_SUFFIX_MSIME;
        //$ case: read input file as Microsoft IME dictionary.
        }else if(
            argv[i] == Opt::STR_READ_MSIME_SHORT ||
            argv[i] == Opt::STR_READ_MSIME_LONG
        ){
            convert = read_ms;
            outputFilename_suffix = OUTPUT_FILENAME_SUFFIX_CANNA;
        //$ case: output filename specified.
        }else if(
            argv[i] == Opt::STR_OUTPUT_FILENAME_SHORT ||
            std::string(argv[i]).substr(0, Opt::STR_OUTPUT_FILENAME_LONG.length())
                == Opt::STR_OUTPUT_FILENAME_LONG
        ){
            outputFilename = argv[i++];
        //$ case: input file name specified.
        }else{
            inputFilename = argv[i];
        }
    }

    if(outputFilename.empty()) {
        const size_t lastDotIndex = inputFilename.find_last_of(".");
        if(lastDotIndex != std::string::npos) {
            outputFilename.reserve(inputFilename.length() + outputFilename_suffix.length());
            outputFilename = inputFilename.substr(0, lastDotIndex) + outputFilename_suffix;
        } // no_else
    }

try{
    cnm_throw(convert == nullptr,
        "ERROR: please specify input file format (-c or -m)!"
    );
    cnm_throw(inputFilename.empty(), "ERROR: no input file specified!");

    convert(inputFilename.c_str(), outputFilename, tangos);

    std::cout << "Bye.\n";
    return 0;
}catch(const std::runtime_error& err){
    std::cerr << err.what() << '\n';
    return 1;
}}

///////////////////////////////////////////////////////////////

/*static*/ uint16_t Tango::num_converted = 0;

bool Tango::is_valid() const
{return(
    value.length() && furigana.length()
);}


Hinshi2String::Base::Base() {}
/*virtual*/ Hinshi2String::Base::~Base() {}


const std::string& Hinshi2String::Base
    ::operator[](Hinshi wHinshi) const
{
    return data_.find(wHinshi)->second;
}


Hinshi2String::Canna::Canna() : Hinshi2String::Base()
{
    for(const auto& h : HINSHI_TABLE_CANNA) {
        data_[h.second] = h.first;
    }
}
/*virtual*/ Hinshi2String::Canna::~Canna() {}


Hinshi2String::MSIME::MSIME() : Hinshi2String::Base()
{
    for(const auto& h : HINSHI_TABLE_MSIME) {
        data_[h.second] = h.first;
    }

    data_[Hinshi::FUKASHI]              = HINSHI_STR_MSIME_MEISHI;
    data_[Hinshi::FUKUSHI_SURU]         =
    data_[Hinshi::FUKUSHI_TO]           =
    data_[Hinshi::FUKUSHI_TO_SURU]      = HINSHI_STR_MSIME_FUKUSHI;
    data_[Hinshi::JIDOUSHI_BA5]         = HINSHI_STR_MSIME_DOUSHI_BA5;
    data_[Hinshi::JIDOUSHI_NA5]         = HINSHI_STR_MSIME_DOUSHI_NA5;
    data_[Hinshi::JIDOUSHI_SA5]         = HINSHI_STR_MSIME_DOUSHI_SA5;
    data_[Hinshi::JIDOUSHI_TA5]         = HINSHI_STR_MSIME_DOUSHI_TA5;
    data_[Hinshi::KANJI_TAN]            = HINSHI_STR_MSIME_KANJI_TAN;
    data_[Hinshi::KANDOUSHI]            = HINSHI_STR_MSIME_KANDOUSHI;
    data_[Hinshi::MEI_KAISHA]           = HINSHI_STR_MSIME_KOYUU_MEISHI; // lossy: company name -> special name
    data_[Hinshi::MEISHI_NA_SA]         = HINSHI_STR_MSIME_MEISHI;  // lossy: na,sa noun -> general noun
    data_[Hinshi::MEISHI_NA_SA_SURU]    = HINSHI_STR_MSIME_MEISHI;  // lossy: na,sa,suru noun -> general noun
    data_[Hinshi::MEISHI_NA_SURU]       = HINSHI_STR_MSIME_MEISHI;  // lossy: na,suru noun -> general noun
    data_[Hinshi::TADOUSHI_BA5]         = HINSHI_STR_MSIME_DOUSHI_BA5; // lossy: ba5 transitive -> ba5
    data_[Hinshi::TADOUSHI_GA5]         = HINSHI_STR_MSIME_DOUSHI_GA5; // lossy: ga5 transitive -> ga5
    data_[Hinshi::TADOUSHI_KA5]         = HINSHI_STR_MSIME_DOUSHI_KA5; // lossy: ka5 transitive -> ka5
    data_[Hinshi::TADOUSHI_MA5]         = HINSHI_STR_MSIME_DOUSHI_MA5; // lossy: ma5 transitive -> ma5
    data_[Hinshi::TADOUSHI_NA5]         = HINSHI_STR_MSIME_DOUSHI_NA5; // lossy: na5 transitive -> na5
    data_[Hinshi::TADOUSHI_RA5]         = HINSHI_STR_MSIME_DOUSHI_RA5; // lossy: ra5 transitive -> ra5
    data_[Hinshi::TADOUSHI_SA5]         = HINSHI_STR_MSIME_DOUSHI_SA5; // lossy: sa5 transitive -> sa5
    data_[Hinshi::TADOUSHI_TA5]         = HINSHI_STR_MSIME_DOUSHI_TA5; // lossy: ta5 transitive -> ta5
    data_[Hinshi::TADOUSHI_WA5]         = HINSHI_STR_MSIME_DOUSHI_WA5; // lossy: wa5 transitive -> wa5
    data_[Hinshi::UNKNOWN]              = HINSHI_STR_UNKNOWN;

    cnm_throw(data_.size() != HINSHI_NAMES.size(), "Hinshi2String::MSIME data incomplete!");
}
Hinshi2String::MSIME::~MSIME() {}
