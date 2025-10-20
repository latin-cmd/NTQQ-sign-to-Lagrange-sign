#include "server.h"
#include "../include/rapidjson/document.h"
#include "../include/rapidjson/writer.h"

std::string Server::GetSign(const std::string_view &cmd, const std::string_view &src, const int seq)
{
    auto [signDataHex, extraDataHex, tokenDataHex] = Sign::Call(cmd, src, seq);
    counter++;

    // 动态识别当前系统平台
    const char* platform = 
#if defined(_WIN32) || defined(_WIN64)
        "Windows";
#elif defined(__APPLE__) || defined(__MACH__)
        "macOS";
#elif defined(__linux__)
        "Linux";
#else
        "Unknown";
#endif

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

    writer.StartObject();
    
    // 平台标识
    writer.Key("platform");
    writer.String(platform);
    
    // 签名数据
    writer.Key("value");
    writer.StartObject();
    writer.Key("extra");
    writer.String(extraDataHex.c_str());
    writer.Key("token");
    writer.String(tokenDataHex.c_str());
    writer.Key("sign");
    writer.String(signDataHex.c_str());
    writer.EndObject();
    
    writer.EndObject();

    return buffer.GetString();
}

void Server::Init()
{
    svr.Get("/", [](const httplib::Request &req, httplib::Response &res)
             {
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

            writer.StartObject();
            writer.Key("Msg");
            writer.String("TsukishiroStudio/SignerServer");
            writer.Key("ok");
            writer.String("true");
            writer.Key("mod");
            writer.String("Lagrange.Core");
            writer.EndObject();

            res.set_content(buffer.GetString(), "application/json"); })
        .Post("/sign", [this](const httplib::Request &req, httplib::Response &res)
             {
            try {
                rapidjson::Document doc;
                doc.Parse(req.body.c_str(), req.body.size());

                std::string_view cmd = doc["cmd"].GetString();
                std::string_view src = doc["src"].GetString();
                int seq = doc["seq"].GetInt();

                std::string buffer = GetSign(cmd, src, seq);

                res.set_content(buffer, "application/json");
            }
            catch (...) {
                res.set_content("Bad Request", "text/plain");
                res.status = httplib::StatusCode::BadRequest_400;
            } })
        .Get("/sign", [this](const httplib::Request &req, httplib::Response &res)
             {
            try {
                std::string_view cmd = req.get_param_value("cmd");
                std::string_view src = req.get_param_value("src");
                int seq = std::stoi(req.get_param_value("seq"));

                std::string buffer = GetSign(cmd, src, seq);

                res.set_content(buffer, "application/json");
            }
            catch (...) {
                res.set_content("Bad Request", "text/plain");
                res.status = httplib::StatusCode::BadRequest_400;
            } })
        .Get("/ping", [](const httplib::Request &req, httplib::Response &res)
             {
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

            writer.StartObject();
            writer.Key("code");
            writer.Int(0);
            writer.EndObject();

            res.set_content(buffer.GetString(), "application/json"); })
        .Get("/count", [this](const httplib::Request &req, httplib::Response &res)
             {
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

            writer.StartObject();
            writer.Key("count");
            writer.String(std::to_string(counter.load()).c_str());
            writer.EndObject();

            res.set_content(buffer.GetString(), "application/json"); });
}

bool Server::Run(const std::string &ip, int port)
{
    return svr.listen(ip.c_str(), port);
}
