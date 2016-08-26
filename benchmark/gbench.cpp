#include <benchmark/benchmark_api.h>
#include <rapidhttp/document.h>
#include <stdio.h>
#if PROFILE
#include <gperftools/profiler.h>
#endif

static std::string c_http_request_0 = 
"GET /uri/abc HTTP/1.1\r\n"
"\r\n";

static std::string c_http_request_1 = 
"GET /uri/abc HTTP/1.1\r\n"
"Accept: XAccept\r\n"
"\r\n";

static std::string c_http_request_2 = 
"GET /uri/abc HTTP/1.1\r\n"
"Accept: XAccept\r\n"
"Host: domain.com\r\n"
"\r\n";

static std::string c_http_request = 
"GET /uri/abc HTTP/1.1\r\n"
"Accept: XAccept\r\n"
"Host: domain.com\r\n"
"Connection: Keep-Alive\r\n"
"\r\n";

static std::string c_http_response = 
"HTTP/1.1 200 OK\r\n"
"Accept: XAccept\r\n"
"Host: domain.com\r\n"
"Content-Length: 3\r\n"
"\r\nabc";

static std::string c_big_request =
"POST /joyent/http-parser HTTP/1.1\r\n"
"Host: github.com\r\n"
"DNT: 1\r\n"
"Accept-Encoding: gzip, deflate, sdch\r\n"
"Accept-Language: ru-RU,ru;q=0.8,en-US;q=0.6,en;q=0.4\r\n"
"User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_10_1) "
    "AppleWebKit/537.36 (KHTML, like Gecko) "
    "Chrome/39.0.2171.65 Safari/537.36\r\n"
"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,"
    "image/webp,*/*;q=0.8\r\n"
"Referer: https://github.com/joyent/http-parser\r\n"
"Connection: keep-alive\r\n"
"Transfer-Encoding: chunked\r\n"
"Cache-Control: max-age=0\r\n\r\nb\r\nhello world\r\n0\r\n\r\n";

template <class DocType> void BM_ParseRequest_0_field(benchmark::State& state)
{
    while (state.KeepRunning()) {
        for (int x = 0; x < state.range(0); ++x) {
            DocType doc(rapidhttp::Request);
            size_t bytes = doc.PartailParse(c_http_request_0);
            (void)bytes;
//            printf("parse bytes: %d. error:%s\n", (int)bytes, doc.ParseError().message().c_str());
        }
    }
}

template <class DocType> void BM_ParseRequest_1_field(benchmark::State& state)
{
    while (state.KeepRunning()) {
        for (int x = 0; x < state.range(0); ++x) {
            DocType doc(rapidhttp::Request);
            size_t bytes = doc.PartailParse(c_http_request_1);
            (void)bytes;
//            printf("parse bytes: %d. error:%s\n", (int)bytes, doc.ParseError().message().c_str());
        }
    }
}

template <class DocType> void BM_ParseRequest_2_field(benchmark::State& state)
{
    while (state.KeepRunning()) {
        for (int x = 0; x < state.range(0); ++x) {
            DocType doc(rapidhttp::Request);
            size_t bytes = doc.PartailParse(c_http_request_2);
            (void)bytes;
//            printf("parse bytes: %d. error:%s\n", (int)bytes, doc.ParseError().message().c_str());
        }
    }
}

template <class DocType> void BM_ParseRequest_3_field(benchmark::State& state)
{
    while (state.KeepRunning()) {
        for (int x = 0; x < state.range(0); ++x) {
            DocType doc(rapidhttp::Request);
            size_t bytes = doc.PartailParse(c_http_request.c_str(), c_http_request.size());
            (void)bytes;
//            printf("parse bytes: %d. error:%s\n", (int)bytes, doc.ParseError().message().c_str());
        }
    }
}

template <class DocType> void BM_ParseRequest_big(benchmark::State& state)
{
    while (state.KeepRunning()) {
        for (int x = 0; x < state.range(0); ++x) {
            DocType doc(rapidhttp::Request);
            size_t bytes = doc.PartailParse(c_big_request);
            (void)bytes;
//            printf("parse bytes: %d. error:%s\n", (int)bytes, doc.ParseError().message().c_str());
        }
    }
}

template <class DocType> void BM_ParseResponse(benchmark::State& state)
{
    while (state.KeepRunning()) {
        for (int x = 0; x < state.range(0); ++x) {
            DocType doc(rapidhttp::Response);
            size_t bytes = doc.PartailParse(c_http_response.c_str(), c_http_response.size());
            (void)bytes;
//            printf("parse bytes: %d. error:%s\n", (int)bytes, doc.ParseError().message().c_str());
        }
    }
}

template <typename DocType>
DocType& GetDoc()
{
    static DocType doc(rapidhttp::Response);
    return doc;
}

template <class DocType> void BM_PartialParseResponse(benchmark::State& state)
{
    while (state.KeepRunning()) {
        for (int x = 0; x < state.range(0); ++x) {
            auto & doc = GetDoc<DocType>();
            size_t bytes = doc.PartailParse(c_http_response.c_str(), c_http_response.size() / 2);
            bytes += doc.PartailParse(c_http_response.c_str() + bytes, c_http_response.size() - bytes);
//            printf("parse bytes: %d. error:%s done:%d\n", (int)bytes, doc.ParseError().message().c_str(), doc.ParseDone());
        }
    }
}

template <class DocType> void BM_Serialize(benchmark::State& state)
{
    while (state.KeepRunning()) {
        for (int x = 0; x < state.range(0); ++x) {
            auto & doc = GetDoc<DocType>();
            char buf[128] = {};
            bool b = doc.Serialize(buf, sizeof(buf));
            (void)b;
//            printf("response:\n%s\nByteSize:%d\n", buf, (int)doc.ByteSize());
//            exit(0);
//            printf("serialize ok: %d\n", b);
        }
    }
}

template <class Src, class Dst> void BM_CopyTo(benchmark::State& state)
{
    while (state.KeepRunning()) {
        for (int x = 0; x < state.range(0); ++x) {
            auto & src = GetDoc<Src>();
            auto & dst = GetDoc<Dst>();
            src.CopyTo(dst);
        }
    }
}

BENCHMARK_TEMPLATE(BM_ParseRequest_0_field, rapidhttp::HttpDocument)->Arg(1);
BENCHMARK_TEMPLATE(BM_ParseRequest_1_field, rapidhttp::HttpDocument)->Arg(1);
BENCHMARK_TEMPLATE(BM_ParseRequest_2_field, rapidhttp::HttpDocument)->Arg(1);
BENCHMARK_TEMPLATE(BM_ParseRequest_3_field, rapidhttp::HttpDocument)->Arg(1);
BENCHMARK_TEMPLATE(BM_ParseRequest_big, rapidhttp::HttpDocument)->Arg(1);
BENCHMARK_TEMPLATE(BM_ParseResponse, rapidhttp::HttpDocument)->Arg(1);
BENCHMARK_TEMPLATE(BM_PartialParseResponse, rapidhttp::HttpDocument)->Arg(1);
BENCHMARK_TEMPLATE(BM_Serialize, rapidhttp::HttpDocument)->Arg(1);

BENCHMARK_TEMPLATE(BM_ParseRequest_0_field, rapidhttp::HttpDocumentRef)->Arg(1);
BENCHMARK_TEMPLATE(BM_ParseRequest_1_field, rapidhttp::HttpDocumentRef)->Arg(1);
BENCHMARK_TEMPLATE(BM_ParseRequest_2_field, rapidhttp::HttpDocumentRef)->Arg(1);
BENCHMARK_TEMPLATE(BM_ParseRequest_3_field, rapidhttp::HttpDocumentRef)->Arg(1);
BENCHMARK_TEMPLATE(BM_ParseRequest_big, rapidhttp::HttpDocumentRef)->Arg(1);
BENCHMARK_TEMPLATE(BM_ParseResponse, rapidhttp::HttpDocumentRef)->Arg(1);
BENCHMARK_TEMPLATE(BM_PartialParseResponse, rapidhttp::HttpDocumentRef)->Arg(1);
BENCHMARK_TEMPLATE(BM_Serialize, rapidhttp::HttpDocumentRef)->Arg(1);

BENCHMARK_TEMPLATE(BM_CopyTo, rapidhttp::HttpDocumentRef, rapidhttp::HttpDocument)->Arg(1);
BENCHMARK_TEMPLATE(BM_CopyTo, rapidhttp::HttpDocumentRef, rapidhttp::HttpDocumentRef)->Arg(1);
BENCHMARK_TEMPLATE(BM_CopyTo, rapidhttp::HttpDocument, rapidhttp::HttpDocumentRef)->Arg(1);
BENCHMARK_TEMPLATE(BM_CopyTo, rapidhttp::HttpDocument, rapidhttp::HttpDocument)->Arg(1);

int main(int argc, char** argv) {
    ::benchmark::Initialize(&argc, argv);
#if PROFILE
    ProfilerStart("bench.prof");
#endif
    ::benchmark::RunSpecifiedBenchmarks();
#if PROFILE
    ProfilerStop();
#endif
}
