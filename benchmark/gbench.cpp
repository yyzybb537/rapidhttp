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

void BM_ParseRequest_0_field(benchmark::State& state)
{
    while (state.KeepRunning()) {
        for (int x = 0; x < state.range(0); ++x) {
            rapidhttp::HttpDocument doc(rapidhttp::HttpDocument::Request);
            size_t bytes = doc.PartailParse(c_http_request_0);
            (void)bytes;
//            printf("parse bytes: %d. error:%s\n", (int)bytes, doc.ParseError().message().c_str());
        }
    }
}
BENCHMARK(BM_ParseRequest_0_field)->Arg(1);

void BM_ParseRequest_1_field(benchmark::State& state)
{
    while (state.KeepRunning()) {
        for (int x = 0; x < state.range(0); ++x) {
            rapidhttp::HttpDocument doc(rapidhttp::HttpDocument::Request);
            size_t bytes = doc.PartailParse(c_http_request_1);
            (void)bytes;
//            printf("parse bytes: %d. error:%s\n", (int)bytes, doc.ParseError().message().c_str());
        }
    }
}
BENCHMARK(BM_ParseRequest_1_field)->Arg(1);

void BM_ParseRequest_2_field(benchmark::State& state)
{
    while (state.KeepRunning()) {
        for (int x = 0; x < state.range(0); ++x) {
            rapidhttp::HttpDocument doc(rapidhttp::HttpDocument::Request);
            size_t bytes = doc.PartailParse(c_http_request_2);
            (void)bytes;
//            printf("parse bytes: %d. error:%s\n", (int)bytes, doc.ParseError().message().c_str());
        }
    }
}
BENCHMARK(BM_ParseRequest_2_field)->Arg(1);

void BM_ParseRequest_3_field(benchmark::State& state)
{
    while (state.KeepRunning()) {
        for (int x = 0; x < state.range(0); ++x) {
            rapidhttp::HttpDocument doc(rapidhttp::HttpDocument::Request);
            size_t bytes = doc.PartailParse(c_http_request.c_str(), c_http_request.size());
            (void)bytes;
//            printf("parse bytes: %d. error:%s\n", (int)bytes, doc.ParseError().message().c_str());
        }
    }
}
BENCHMARK(BM_ParseRequest_3_field)->Arg(1);

void BM_ParseRequest_big(benchmark::State& state)
{
    while (state.KeepRunning()) {
        for (int x = 0; x < state.range(0); ++x) {
            rapidhttp::HttpDocument doc(rapidhttp::HttpDocument::Request);
            size_t bytes = doc.PartailParse(c_big_request);
            (void)bytes;
//            printf("parse bytes: %d. error:%s\n", (int)bytes, doc.ParseError().message().c_str());
        }
    }
}
BENCHMARK(BM_ParseRequest_big)->Arg(1)->Arg(10<<10);

void BM_ParseResponse(benchmark::State& state)
{
    while (state.KeepRunning()) {
        for (int x = 0; x < state.range(0); ++x) {
            rapidhttp::HttpDocument doc(rapidhttp::HttpDocument::Response);
            size_t bytes = doc.PartailParse(c_http_response.c_str(), c_http_response.size());
            (void)bytes;
//            printf("parse bytes: %d. error:%s\n", (int)bytes, doc.ParseError().message().c_str());
        }
    }
}
BENCHMARK(BM_ParseResponse)->Arg(1);

rapidhttp::HttpDocument doc(rapidhttp::HttpDocument::Response);

void BM_PartialParseResponse(benchmark::State& state)
{
    while (state.KeepRunning()) {
        for (int x = 0; x < state.range(0); ++x) {
            size_t bytes = doc.PartailParse(c_http_response.c_str(), c_http_response.size() / 2);
            bytes += doc.PartailParse(c_http_response.c_str() + bytes, c_http_response.size() - bytes);
//            printf("parse bytes: %d. error:%s done:%d\n", (int)bytes, doc.ParseError().message().c_str(), doc.ParseDone());
        }
    }
}
BENCHMARK(BM_PartialParseResponse)->Arg(1);

void BM_Serialize(benchmark::State& state)
{
    while (state.KeepRunning()) {
        for (int x = 0; x < state.range(0); ++x) {
            char buf[128] = {};
            bool b = doc.Serialize(buf, sizeof(buf));
            (void)b;
//            printf("response:\n%s\nByteSize:%d\n", buf, (int)doc.ByteSize());
//            exit(0);
//            printf("serialize ok: %d\n", b);
        }
    }
}
BENCHMARK(BM_Serialize)->Arg(1);

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
