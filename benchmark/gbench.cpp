#include <benchmark/benchmark_api.h>
#include <rapidhttp/document.h>
#include <stdio.h>

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
"Connection: Keep-Alive\r\n"
"\r\n";

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
            bytes += doc.PartailParse(c_http_response.c_str() + c_http_response.size() / 2, c_http_response.size() - c_http_response.size() / 2);
//            printf("parse bytes: %d. error:%s\n", (int)bytes, doc.ParseError().message().c_str());
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

BENCHMARK_MAIN()
