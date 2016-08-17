#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>

#include <vector>
#include <string>
#include <stdint.h>
#include <time.h>
#include <algorithm>

const char* ssid = "SSID";
const char* password = "PASSWORD";

static char const consumer_key[]    = "XXXXXXXXXXXXXXXXXXXXXX";
static char const consumer_sec[]    = "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
static char const accesstoken[]     = "0000000000-XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
static char const accesstoken_sec[] = "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";

time_t timevalue = 0;

class misc {
private:
  static void url_encode_(const char *ptr, const char *end, std::vector<char> *out)
  {
    while (ptr < end) {
      int c = (unsigned char)*ptr;
      ptr++;
      if (isalnum(c) || strchr("_.-~", c)) {
        print(out, c);
      } else if (c == ' ') {
        print(out, '+');
      } else {
        char tmp[10];
        sprintf(tmp, "%%%02X", c);
        print(out, tmp[0]);
        print(out, tmp[1]);
        print(out, tmp[2]);
      }
    }
  }
  static void url_decode_(const char *ptr, const char *end, std::vector<char> *out)
  {
    while (ptr < end) {
      int c = (unsigned char)*ptr;
      ptr++;
      if (c == '+') {
        c = ' ';
      } else if (c == '%' && isxdigit((unsigned char)ptr[0]) && isxdigit((unsigned char)ptr[1])) {
        char tmp[3]; // '%XX'
        tmp[0] = ptr[0];
        tmp[1] = ptr[1];
        tmp[2] = 0;
        c = strtol(tmp, NULL, 16);
        ptr += 2;
      }
      print(out, c);
    }
  }
public:
  static void print(std::vector<char> *out, char c)
  {
    out->push_back(c);
  }
  static void print(std::vector<char> *out, char const *begin, char const *end)
  {
    out->insert(out->end(), begin, end);
  }
  static void print(std::vector<char> *out, char const *ptr, size_t len)
  {
    print(out, ptr, ptr + len);
  }
  static void print(std::vector<char> *out, char const *s)
  {
    print(out, s, s + strlen(s));
  }
  static void print(std::vector<char> *out, std::string const &s)
  {
    print(out, s.c_str(), s.size());
  }
  static std::string to_stdstr(std::vector<char> const &vec)
  {
    if (!vec.empty()) {
      char const *begin = &vec.at(0);
      char const *end = begin + vec.size();
      return std::string(begin, end);
    }
    return std::string();
  }

  static std::string url_encode(char const *str, char const *end)
  {
    if (!str) {
      return std::string();
    }

    std::vector<char> out;
    out.reserve(end - str + 10);

    url_encode_(str, end, &out);

    return to_stdstr(out);
  }
  static std::string url_decode(char const *str, char const *end)
  {
    if (!str) {
      return std::string();
    }

    std::vector<char> out;
    out.reserve(end - str + 10);

    url_decode_(str, end, &out);

    return to_stdstr(out);
  }

  static std::string url_encode(char const *str, size_t len)
  {
    return url_encode(str, str + len);
  }
  static std::string url_decode(char const *str, size_t len)
  {
    return url_decode(str, str + len);
  }

  static std::string url_encode(char const *str)
  {
    return url_encode(str, strlen(str));
  }
  static std::string url_decode(const char *str)
  {
    return url_decode(str, strlen(str));
  }

  static std::string url_encode(std::string const &str)
  {
    char const *begin = str.c_str();
    char const *end = begin + str.size();
    char const *ptr = begin;

    while (ptr < end) {
      int c = (unsigned char)*ptr;
      if (isalnum(c) || strchr("_.-~", c)) {
        // thru
      } else {
        break;
      }
      ptr++;
    }
    if (ptr == end) {
      return str;
    }

    std::vector<char> out;
    out.reserve(str.size() + 10);

    out.insert(out.end(), begin, ptr);
    url_encode_(ptr, end, &out);

    return to_stdstr(out);
  }
  static std::string url_decode(std::string const &str)
  {
    char const *begin = str.c_str();
    char const *end = begin + str.size();
    char const *ptr = begin;

    while (ptr < end) {
      int c = *ptr & 0xff;
      if (c == '+' || c == '%') {
        break;
      }
      ptr++;
    }
    if (ptr == end) {
      return str;
    }


    std::vector<char> out;
    out.reserve(str.size() + 10);

    out.insert(out.end(), begin, ptr);
    url_decode_(ptr, end, &out);

    return to_stdstr(out);
  }

};

class base64 {
private:
  static const unsigned char PAD = '=';
  static unsigned char enc(int c)
  {
    static const unsigned char _encode_table[] = {
      0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50,
      0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66,
      0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76,
      0x77, 0x78, 0x79, 0x7a, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x2b, 0x2f,
    };
    return _encode_table[c & 63];
  }
  static unsigned char dec(int c)
  {
    static const unsigned char _decode_table[] = {
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x3e, 0xff, 0xff, 0xff, 0x3f,
      0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0xff, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
      0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0xff, 0xff, 0xff, 0xff, 0xff,
      0xff, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
      0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0xff, 0xff, 0xff, 0xff, 0xff,
    };
    return _decode_table[c & 127];
  }
public:
  static void encode(char const *src, size_t length, std::vector<char> *out)
  {
    size_t srcpos, dstlen, dstpos;

    dstlen = (length + 2) / 3 * 4;
    out->resize(dstlen);
    if (dstlen == 0) {
      return;
    }
    char *dst = &out->at(0);
    dstpos = 0;
    for (srcpos = 0; srcpos < length; srcpos += 3) {
      int v = (unsigned char)src[srcpos] << 16;
      if (srcpos + 1 < length) {
        v |= (unsigned char)src[srcpos + 1] << 8;
        if (srcpos + 2 < length) {
          v |= (unsigned char)src[srcpos + 2];
          dst[dstpos + 3] = enc(v);
        } else {
          dst[dstpos + 3] = PAD;
        }
        dst[dstpos + 2] = enc(v >> 6);
      } else {
        dst[dstpos + 2] = PAD;
        dst[dstpos + 3] = PAD;
      }
      dst[dstpos + 1] = enc(v >> 12);
      dst[dstpos] = enc(v >> 18);
      dstpos += 4;
    }
  }
  static void decode(char const *src, size_t length, std::vector<char> *out)
  {
    unsigned char const *begin = (unsigned char const *)src;
    unsigned char const *end = begin + length;
    unsigned char const *ptr = begin;
    out->clear();
    out->reserve(length * 3 / 4);
    int count = 0;
    int bits = 0;
    while (1) {
      if (isspace(*ptr)) {
        ptr++;
      } else {
        unsigned char c = 0xff;
        if (ptr < end && *ptr < 0x80) {
          c = dec(*ptr);
        }
        if (c < 0x40) {
          bits = (bits << 6) | c;
          count++;
        } else {
          if (count < 4) {
            bits <<= (4 - count) * 6;
          }
          c = 0xff;
        }
        if (count == 4 || c == 0xff) {
          if (count >= 2) {
            out->push_back(bits >> 16);
            if (count >= 3) {
              out->push_back(bits >> 8);
              if (count == 4) {
                out->push_back(bits);
              }
            }
          }
          count = 0;
          bits = 0;
          if (c == 0xff) {
            break;
          }
        }
        ptr++;
      }
    }
  }
  static void encode(std::vector<char> const *src, std::vector<char> *out)
  {
    encode(&src->at(0), src->size(), out);
  }
  static void decode(std::vector<char> const *src, std::vector<char> *out)
  {
    decode(&src->at(0), src->size(), out);
  }
  static void encode(char const *src, std::vector<char> *out)
  {
    encode((char const *)src, strlen(src), out);
  }
  static void decode(char const *src, std::vector<char> *out)
  {
    decode((char const *)src, strlen(src), out);
  }
  static inline std::string to_s_(std::vector<char> const *vec)
  {
    if (!vec || vec->empty()) return std::string();
    return std::string((char const *)&(*vec)[0], vec->size());
  }
  static inline std::string encode(std::string const &src)
  {
    std::vector<char> vec;
    encode((char const *)src.c_str(), src.size(), &vec);
    return to_s_(&vec);
  }
  static inline std::string decode(std::string const &src)
  {
    std::vector<char> vec;
    decode((char const *)src.c_str(), src.size(), &vec);
    return to_s_(&vec);
  }

};

class sha1 {
public:
  enum {
    Success = 0,
    Null,            /* Null pointer parameter */
    InputTooLong,    /* input data too long */
    StateError       /* called Input after Result */
  };
public:
  static const int HashSize = 20;
  struct Context {
    uint32_t Intermediate_Hash[HashSize/4]; /* Message Digest  */

    uint32_t Length_Low;            /* Message length in bits      */
    uint32_t Length_High;           /* Message length in bits      */

    /* Index into message block array   */
    int_least16_t Message_Block_Index;
    uint8_t Message_Block[64];      /* 512-bit message blocks      */

    int Computed;               /* Is the digest computed?         */
    int Corrupted;             /* Is the message digest corrupted? */
  };
  static uint32_t CircularShift(uint32_t bits, uint32_t word)
  {
    return ((word) << (bits)) | ((word) >> (32-(bits)));
  }
  static void PadMessage(Context *context)
  {
    if (context->Message_Block_Index > 55) {
      context->Message_Block[context->Message_Block_Index++] = 0x80;
      while (context->Message_Block_Index < 64) {
        context->Message_Block[context->Message_Block_Index++] = 0;
      }

      ProcessMessageBlock(context);

      while (context->Message_Block_Index < 56) {
        context->Message_Block[context->Message_Block_Index++] = 0;
      }
    } else {
      context->Message_Block[context->Message_Block_Index++] = 0x80;
      while(context->Message_Block_Index < 56) {
        context->Message_Block[context->Message_Block_Index++] = 0;
      }
    }

    context->Message_Block[56] = context->Length_High >> 24;
    context->Message_Block[57] = context->Length_High >> 16;
    context->Message_Block[58] = context->Length_High >> 8;
    context->Message_Block[59] = context->Length_High;
    context->Message_Block[60] = context->Length_Low >> 24;
    context->Message_Block[61] = context->Length_Low >> 16;
    context->Message_Block[62] = context->Length_Low >> 8;
    context->Message_Block[63] = context->Length_Low;

    ProcessMessageBlock(context);
  }
  static void ProcessMessageBlock(Context *context)
  {
    const uint32_t K[] = {
      0x5A827999,
      0x6ED9EBA1,
      0x8F1BBCDC,
      0xCA62C1D6
    };
    int t;
    uint32_t temp;
    uint32_t W[80];
    uint32_t A, B, C, D, E;

    for (t = 0; t < 16; t++) {
      W[t] = context->Message_Block[t * 4] << 24;
      W[t] |= context->Message_Block[t * 4 + 1] << 16;
      W[t] |= context->Message_Block[t * 4 + 2] << 8;
      W[t] |= context->Message_Block[t * 4 + 3];
    }

    for (t = 16; t < 80; t++) {
      W[t] = CircularShift(1,W[t-3] ^ W[t-8] ^ W[t-14] ^ W[t-16]);
    }

    A = context->Intermediate_Hash[0];
    B = context->Intermediate_Hash[1];
    C = context->Intermediate_Hash[2];
    D = context->Intermediate_Hash[3];
    E = context->Intermediate_Hash[4];

    for (t = 0; t < 20; t++) {
      temp =  CircularShift(5,A) + ((B & C) | ((~B) & D)) + E + W[t] + K[0];
      E = D;
      D = C;
      C = CircularShift(30,B);
      B = A;
      A = temp;
    }

    for (t = 20; t < 40; t++) {
      temp = CircularShift(5,A) + (B ^ C ^ D) + E + W[t] + K[1];
      E = D;
      D = C;
      C = CircularShift(30,B);
      B = A;
      A = temp;
    }

    for (t = 40; t < 60; t++) {
      temp = CircularShift(5,A) + ((B & C) | (B & D) | (C & D)) + E + W[t] + K[2];
      E = D;
      D = C;
      C = CircularShift(30,B);
      B = A;
      A = temp;
    }

    for (t = 60; t < 80; t++) {
      temp = CircularShift(5,A) + (B ^ C ^ D) + E + W[t] + K[3];
      E = D;
      D = C;
      C = CircularShift(30,B);
      B = A;
      A = temp;
    }

    context->Intermediate_Hash[0] += A;
    context->Intermediate_Hash[1] += B;
    context->Intermediate_Hash[2] += C;
    context->Intermediate_Hash[3] += D;
    context->Intermediate_Hash[4] += E;

    context->Message_Block_Index = 0;
  }
public:
  static int Reset(Context *context)
  {
    if (!context)
    {
      return Null;
    }

    context->Length_Low             = 0;
    context->Length_High            = 0;
    context->Message_Block_Index    = 0;

    context->Intermediate_Hash[0]   = 0x67452301;
    context->Intermediate_Hash[1]   = 0xEFCDAB89;
    context->Intermediate_Hash[2]   = 0x98BADCFE;
    context->Intermediate_Hash[3]   = 0x10325476;
    context->Intermediate_Hash[4]   = 0xC3D2E1F0;

    context->Computed   = 0;
    context->Corrupted  = 0;

    return Success;
  }
  static int Result(Context *context, uint8_t Message_Digest[])
  {
    int i;

    if (!context || !Message_Digest) {
      return Null;
    }

    if (context->Corrupted) {
      return context->Corrupted;
    }

    if (!context->Computed) {
      PadMessage(context);
      for (i = 0; i < 64; ++i) {
        /* message may be sensitive, clear it out */
        context->Message_Block[i] = 0;
      }
      context->Length_Low = 0;    /* and clear length */
      context->Length_High = 0;
      context->Computed = 1;
    }

    for (i = 0; i < HashSize; ++i)
    {
      Message_Digest[i] = context->Intermediate_Hash[i>>2] >> 8 * ( 3 - ( i & 0x03 ) );
    }

    return Success;
  }
  static int Input(Context *context, const uint8_t *message_array, unsigned int length)
  {
    if (!length) {
      return Success;
    }

    if (!context || !message_array) {
      return Null;
    }

    if (context->Computed) {
      context->Corrupted = StateError;
      return StateError;
    }

    if (context->Corrupted) {
      return context->Corrupted;
    }
    while(length-- && !context->Corrupted) {
      context->Message_Block[context->Message_Block_Index++] = (*message_array & 0xFF);

      context->Length_Low += 8;
      if (context->Length_Low == 0) {
        context->Length_High++;
        if (context->Length_High == 0) {
          /* Message is too long */
          context->Corrupted = 1;
        }
      }

      if (context->Message_Block_Index == 64) {
        ProcessMessageBlock(context);
      }

      message_array++;
    }

    return Success;
  }
};



class oauth {
public:
  class Keys {
  public:
    std::string consumer_key;
    std::string consumer_sec;
    std::string accesstoken;
    std::string accesstoken_sec;

    Keys()
    {
    }

    Keys(std::string consumer_key, std::string consumer_sec, std::string accesstoken, std::string accesstoken_sec)
      : consumer_key(consumer_key)
      , consumer_sec(consumer_sec)
      , accesstoken(accesstoken)
      , accesstoken_sec(accesstoken_sec)
    {
    }
  };
  enum http_method_t {
    GET,
    POST,
  };
  struct Request {
    std::string url;
    std::string post;
  };
  static Request sign(const char *url, http_method_t http_method, Keys const &keys)
  {
    std::vector<std::string> vec;
    split_url(url, &vec);

    process_(&vec, http_method, keys);

    if (http_method == POST) {
      Request req;
      req.post = oauth::build_url(vec, 1);
      req.url = vec.at(0);
      return req;
    } else {
      Request req;
      req.url = oauth::build_url(vec, 0);
      return req;
    }
  }
private:
  static std::string encode_base64(const unsigned char *src, int size)
  {
    std::vector<char> vec;
    base64::encode((char const *)src, size, &vec);
    return misc::to_stdstr(vec);

  }
  static std::string decode_base64(const char *src)
  {
    std::vector<char> vec;
    base64::decode(src, &vec);
    return misc::to_stdstr(vec);
  }
  static std::string url_escape(const char *string)
  {
    return misc::url_encode(string);
  }
  static std::string url_unescape(const char *string)
  {
    return misc::url_decode(string);
  }
  static void process_(std::vector<std::string> *args, http_method_t http_method, const Keys &keys)
  {
    auto to_s = [](int v)->std::string{
      char tmp[100];
      sprintf(tmp, "%d", v);
      return tmp;
    };

    {
      auto nonce = [](){
        static const char *chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";
        const unsigned int max = 26 + 26 + 10 + 1;
        char tmp[50];
//        srand((unsigned int)time(0));
        srand((unsigned int)timevalue);
        int len = 15 + rand() % 16;
        for (int i = 0; i < len; i++) {
          tmp[i] = chars[rand() % max];
        }
        return std::string(tmp, len);
      };

      auto is_key_contains = [](std::vector<std::string> const &argv, std::string const &key)
      {
        size_t keylen = key.size();
        for (std::string const &s : argv) {
          if (strncmp(s.c_str(), key.c_str(), keylen) == 0 && s[keylen] == '=') {
            return true;
          }
        }
        return false;
      };

      std::string oauth_nonce = "oauth_nonce";
      if (!is_key_contains(*args, oauth_nonce)) {
        oauth_nonce += '=';
        oauth_nonce += nonce();
        args->push_back(oauth_nonce);
      }

      std::string oauth_timestamp = "oauth_timestamp";
      if (!is_key_contains(*args, oauth_timestamp)) {
        oauth_timestamp += '=';
//        oauth_timestamp += to_s((int)time(nullptr));
        oauth_timestamp += to_s((int)timevalue);
        args->push_back(oauth_timestamp);
      }

      if (!keys.accesstoken.empty()) {
        std::string oauth_token = "oauth_token";
        oauth_token += '=';
        oauth_token += keys.accesstoken;
        args->push_back(oauth_token);
      }

      std::string oauth_consumer_key = "oauth_consumer_key";
      oauth_consumer_key += '=';
      oauth_consumer_key += keys.consumer_key;
      args->push_back(oauth_consumer_key);

      std::string oauth_signature_method = "oauth_signature_method";
      oauth_signature_method += '=';
      oauth_signature_method += "HMAC-SHA1";
      args->push_back(oauth_signature_method);

      std::string oauth_version = "oauth_version";
      if (!is_key_contains(*args, oauth_version)) {
        oauth_version += '=';
        oauth_version += "1.0";
        args->push_back(oauth_version);
      }

    }
    std::sort(args->begin() + 1, args->end());

    auto Combine = [](std::initializer_list<std::string> list){
      std::string text;
      for (std::string const &s : list) {
        if (!s.empty()) {
          if (!text.empty()) {
            text += '&';
          }
          text += misc::url_encode(s);
        }
      }
      return text;
    };

    std::string query = oauth::build_url(*args, 1);

    std::string httpmethod;
    if (http_method == http_method_t::GET) {
      httpmethod = "GET";
    } else if (http_method == http_method_t::POST) {
      httpmethod = "POST";
    }
    std::string m = Combine({httpmethod, (*args)[0], query});
    std::string k = Combine({std::string(keys.consumer_sec), std::string(keys.accesstoken_sec)});

    std::string oauth_signature = "oauth_signature";
    oauth_signature += '=';
    oauth_signature += sign_hmac_sha1(m, k);

    args->push_back(oauth_signature);
  }
  static void split_url(const char *url, std::vector<std::string> *out)
  {
    out->clear();
    char const *left = url;
    char const *ptr = left;
    while (1) {
      int c = *ptr & 0xff;
      if (c == '&' || c == '?' || c == 0) {
        if (left < ptr) {
          out->push_back(std::string(left, ptr));
        }
        if (c == 0) break;
        ptr++;
        left = ptr;
      } else {
        ptr++;
      }
    }
    for (size_t i = 1; i < out->size(); i++) {
      std::string *p = &out->at(i);
      *p = misc::url_decode(*p);
    }
  }
  static std::string build_url(const std::vector<std::string> &argv, int start)
  {
    const char sep = '&';
    std::string query;
    for (size_t i = start; i < argv.size(); i++) {
      std::string s = argv[i];
      if (i > 0) {
        char const *p = s.c_str();
        char const *e = strchr(p, '=');
        if (e) {
          std::string name(p, e);
          std::string value = e + 1;
          s = name + '=' + misc::url_encode(value);
        } else {
          s += '=';
        }
      }
      if (!query.empty()) {
        query += sep;
      }
      query += s;
    }
    return query;
  }
  static std::string sign_hmac_sha1(const std::string &m, const std::string &k)
  {
    uint8_t result[20];
    hmac_sha1((uint8_t const *)k.c_str(), k.size(), (uint8_t const *)m.c_str(), m.size(), result);
    std::vector<char> vec;
    base64::encode((char const *)result, 20, &vec);
    return misc::to_stdstr(vec);
  }
  static void hmac_sha1(const uint8_t *key, size_t keylen, const uint8_t *in, size_t inlen, uint8_t *resbuf)
  {
    sha1::Context inner;
    sha1::Context outer;
    uint8_t tmpkey[20];
    uint8_t digest[20];
    uint8_t block[64];

    const int IPAD = 0x36;
    const int OPAD = 0x5c;

    if (keylen > 64) {
      struct sha1::Context keyhash;
      sha1::Reset(&keyhash);
      sha1::Input(&keyhash, (uint8_t const *)key, keylen);
      sha1::Result(&keyhash, (uint8_t *)tmpkey);
      key = tmpkey;
      keylen = 20;
    }

    for (size_t i = 0; i < sizeof(block); i++) {
      block[i] = IPAD ^ (i < keylen ? key[i] : 0);
    }
    sha1::Reset(&inner);
    sha1::Input(&inner, (uint8_t const *)block, 64);
    sha1::Input(&inner, (uint8_t const *)in, inlen);
    sha1::Result(&inner, digest);

    for (size_t i = 0; i < sizeof(block); i++) {
      block[i] = OPAD ^ (i < keylen ? key[i] : 0);
    }
    sha1::Reset(&outer);
    sha1::Input(&outer, (uint8_t const *)block, 64);
    sha1::Input(&outer, (uint8_t const *)digest, 20);
    sha1::Result(&outer, (uint8_t *)resbuf);
  }
};

class URL {
private:
  std::string scheme_;
  std::string host_;
  int port_ = 0;
  std::string path_;
public:
  URL(const std::string &url)
  {
    char const *str = url.c_str();
    char const *left;
    char const *right;
    left = str;
    right = strstr(left, "://");
    if (right) {
      scheme_.assign(str, right - str);
      left = right + 3;
    }
    right = strchr(left, '/');
    if (right) {
      char const *p = strchr(left, ':');
      if (p && left < p && p < right) {
        int n = 0;
        char const *q = p + 1;
        while (q < right) {
          if (isdigit(*q & 0xff)) {
            n = n * 10 + (*q - '0');
          } else {
            n = -1;
            break;
          }
          q++;
        }
        host_.assign(left, p - left);
        if (n > 0 && n < 65536) {
          port_ = n;
        }
      } else {
        host_.assign(left, right - left);
      }
      path_ = right;
    }
  }
  std::string const &scheme() const { return scheme_; }
  std::string const &host() const { return host_; }
  int port() const { return port_; }
  std::string const &path() const { return path_; }
  bool isssl() const
  {
    if (scheme() == "https") return true;
    if (scheme() == "http") return false;
    if (port() == 443) return true;
    return false;
  }
};

class TwitterClient {
private:
  struct Data {
    oauth::Keys keys;
  } data;
  oauth::Keys const &keys() const
  {
    return data.keys;
  }
  struct RequestOption {
    enum Method {
      GET,
      POST,
    };
    Method method = GET;
    char const *post_begin = nullptr;
    char const *post_end = nullptr;
    std::string upload_path;
    void set_post_data(char const *begin, char const *end)
    {
      post_begin = begin;
      post_end = end;
      method = POST;
    }
    void set_upload_path(std::string const &path)
    {
      upload_path = path;
    }
  };
  bool request(const std::string &url, RequestOption const &opt, std::string *reply)
  {
    if (reply) {
      reply->clear();
    }
    if (opt.method == RequestOption::GET) {
      // not implemented
    } else if (opt.method == RequestOption::POST) {
      if (opt.upload_path.empty()) {
        String host, path;
        int port = 0;
        {
          URL l(url);
          host = l.host().c_str();
          path = l.path().c_str();
          port = l.port();
        }
        if (port == 0) port = 443;
        WiFiClientSecure client;
        if (client.connect(host.c_str(), port)) {
          int len = opt.post_end - opt.post_begin;
          client.println("POST " + path + " HTTP/1.1");
          client.println("Host: " + host);
          client.println("User-Agent: ESP8266");
          client.println("Connection: close");
          client.println("Content-Type: application/x-www-form-urlencoded;");
          client.print("Content-Length: ");
          client.println(len);
          client.println();
          client.write((uint8_t const *)opt.post_begin, len);
//          delay(1000);
          String s = client.readString();
          *reply = std::string(s.c_str(), s.length());
//          Serial.print(reply->c_str());
          Serial.println("Tweeted");
          return true;
        }
      } else {
        // not implemented
      }
    }
    return false;
  }
  static std::string make_authorization_string(const char *begin, const char *end)
  {
    std::vector<char> vec;
    char const *ptr = begin;
    char const *left = begin;
    while (1) {
      int c = -1;
      if (ptr < end) {
        c = *ptr & 0xff;
      }
      if (c == '&' || c == -1) {
        if (left < ptr) {
          if (!vec.empty()) {
            vec.push_back(',');
          }
          std::string s(left, ptr);
          misc::print(&vec, s);
        }
        if (c == -1) break;
        ptr++;
        left = ptr;
      } else {
        ptr++;
      }
    }
    return misc::to_stdstr(vec);
  }
  static std::string make_boundary(const char *begin, const char *end)
  {
    std::string boundary = "-";
    size_t pos = 0;
    while (begin + pos + boundary.size() < end) {
      if (memcmp(begin + pos, boundary.c_str(), boundary.size()) == 0) {
        int i = pos + boundary.size();
        int c = begin[i] & 0xff;
        if (isdigit(c)) {
          c = (c -'0' + 1) % 10;
        } else {
          c = 0;
        }
        boundary += c + '0';
      }
      pos++;
    }
    boundary += '-';
    return boundary;
  }
public:
  TwitterClient()
  {
  }
  TwitterClient(std::string const &consumer_key, std::string const &consumer_sec, std::string const &accesstoken, std::string const &accesstoken_sec)
  {
    data.keys.consumer_key = consumer_key;
    data.keys.consumer_sec = consumer_sec;
    data.keys.accesstoken = accesstoken;
    data.keys.accesstoken_sec = accesstoken_sec;
  }
  bool tweet(std::string message, const std::vector<std::string> *media_ids = nullptr)
  {
    if (message.empty()) {
      return false;
    }

    std::string url = "https://api.twitter.com/1.1/statuses/update.json";

    url += "?status=";
    url += misc::url_encode(message);
    if (media_ids) {
      std::string ids;
      for (std::string const &media_id : *media_ids) {
        if (!media_id.empty()) {
          if (!ids.empty()) {
            ids += ',';
          }
          ids += media_id;
        }
      }
      url += "&media_ids=";
      url += ids;
    }

    oauth::Request oauth_req = oauth::sign(url.c_str(), oauth::POST, keys());
    std::string res;
    RequestOption opt;
    char const *p = oauth_req.post.c_str();
    opt.set_post_data(p, p + oauth_req.post.size());
    return request(oauth_req.url, opt, &res);
  }
};





char ntp_server[] = "ntp.nict.jp";
#define TIMEZONE (9 * 60 * 60)
unsigned int ntp_local_port = 12300;
IPAddress ntp_server_addr;
const int NTP_PACKET_SIZE = 48;
WiFiUDP ntp_udp;
uint64_t millis64 = 0;

// julian to gregorian
void calJtoG(unsigned long j, int *year, int *month, int *day)
{
  int y, m, d;
  y = (j * 4 + 128179) / 146097;
  d = (j * 4 - y * 146097 + 128179) / 4 * 4 + 3;
  j = d / 1461;
  d = (d - j * 1461) / 4 * 5 + 2;
  m = d / 153;
  d = (d - m * 153) / 5 + 1;
  y = (y - 48) * 100 + j;
  if (m < 10) {
    m += 3;
  } else {
    m -= 9;
    y++;
  }
  *year = y;
  *month = m;
  *day = d;
}

void send_ntp_request()
{
  byte buf[NTP_PACKET_SIZE];
  memset(buf, 0, NTP_PACKET_SIZE);
  buf[0] = 0xdb;
  ntp_udp.beginPacket(ntp_server_addr, 123); // NTP requests are to port 123
  ntp_udp.write(buf, NTP_PACKET_SIZE);
  ntp_udp.endPacket();
}

uint64_t recv_udp_packet()
{
  int n = ntp_udp.parsePacket();
  if (n < NTP_PACKET_SIZE) return 0;
  byte buf[NTP_PACKET_SIZE];
  ntp_udp.read(buf, NTP_PACKET_SIZE); // read the packet into the buffer
  uint32_t s = (((unsigned long)buf[0x28] << 24) | ((unsigned long)buf[0x29] << 16) | ((unsigned long)buf[0x2a] << 8) | (unsigned long)buf[0x2b]);
  uint32_t m = (((unsigned long)buf[0x2c] << 24) | ((unsigned long)buf[0x2d] << 16) | ((unsigned long)buf[0x2e] << 8) | (unsigned long)buf[0x2f]);
  m = ((m >> 16) * 1000 + 32768) >> 16;
  return (uint64_t)s * 1000 + m;
}


ESP8266WebServer server(80);

void handleRoot() {
  String t;
  t += "<html>";
  t += "<head>";
  t += "<meta http-equiv='Content-Type' content='text/html; charset=utf-8' />";
  t += "</had>";
  t += "<body>";
  t += "<h3>";
  t += "ESP8266 Twitter bot test";
  t += "</h3>";
  t += "<form method='POST' action='/tweet'>";
  t += "<input type=text name=text style='width: 40em;'>";
  t += "<input type=submit name=submit value='Tweet'>";
  t += "</form>";
  t += "</body>";
  t += "</html>";
  server.send(200, "text/html", t);
}

void handleTweet() {
  if (server.method() == HTTP_POST) {
    std::string text;
    bool submit = false;
    for (uint8_t i=0; i<server.args(); i++){
      if (server.argName(i) == "text") {
        String s = server.arg(i);
        text = std::string(s.c_str(), s.length());
      } else if (server.argName(i) == "submit") {
        submit = true;
      }
    }

   if (submit && !text.empty()) {
      TwitterClient tc(consumer_key, consumer_sec, accesstoken, accesstoken_sec);
      tc.tweet(text);
   }

    server.sendHeader("Location", "/", true);
    server.send(302, "text/plain", "");
  }
}

void handleNotFound(){
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}


void setup(void){
  millis64 = millis();
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  WiFi.hostByName(ntp_server, ntp_server_addr);
  Serial.print("NTP server address: ");
  Serial.println(ntp_server_addr);

  Serial.println("Starting UDP");
  ntp_udp.begin(ntp_local_port);

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);
  server.on("/tweet", handleTweet);

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
}

void processTime()
{
  static int timeout = 0;
  static bool query_sent = false;
  static uint64_t seconds = 0;
  static uint64_t offset = 0;

  uint32_t m = millis();
  uint32_t n = (uint32_t)millis64;
  if (m != n) {
    if (m < n) {
      millis64 += 0x100000000;
    }
    millis64 = (millis64 & 0xffffffff00000000) | m;
  }

  if (timeout == 0) {
    Serial.println("Send NTP request");
    send_ntp_request();
    query_sent = true;
    timeout = 60;
  } else {
    uint64_t t = recv_udp_packet(); // seconds since 1900/01/01
    if (t != 0) {
      Serial.println("Recv NTP response");
      query_sent = false;
      offset = t + (uint64_t)TIMEZONE * 1000 - millis64;
      timeout = 60 * 37;
    }
    uint64_t s = (millis64 + offset) / 1000;
    if (seconds < s) {
//    Serial.print("*");
      seconds = s;
      if (offset > 0) {
        s -= (uint64_t)25567 * 24 * 60 * 60; // shift 70yrs (to unix time)
        timevalue = (time_t)s;
      }
      timeout--;
    }
  }
}

void loop(void){
  processTime();

  server.handleClient();
}

