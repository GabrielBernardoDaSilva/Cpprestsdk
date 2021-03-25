#include <cpprest/http_listener.h>
#include <cpprest/json.h>
#include <iostream>
#include <map>
#include <set>
#include <string>

#pragma comment(lib, "cpprest_2_10")

using namespace web;
using namespace web::http;
using namespace web::http::experimental::listener;
using namespace std;

#define TRACE(msg)            wcout << msg


map<utility::string_t, utility::string_t> dictionary;

void display_json(
        json::value const &jvalue,
        const wchar_t *prefix) {
    wcout << prefix << jvalue.serialize().c_str() << endl;
}

void handle_get(http_request const &request) {
    TRACE(L"\nhandle GET\n");
    string query = request.relative_uri().query();
    auto answer = json::value::object();
    if (query.empty()) {
        string key = query.replace(query.begin(), query.begin() + 4, "");
        auto pos = dictionary.find(key);
        if (pos == dictionary.end()) {
            answer = json::value::string("Not Found");
            request.reply(status_codes::NotFound, answer);
        } else
            answer[pos->first] = json::value::string(pos->second);
    } else {
        for (auto const &p: dictionary) {
            answer[p.first] = json::value::string(p.second);
        }
    }


    display_json(json::value::null(), L"R: ");
    display_json(answer, L"S: ");

    request.reply(status_codes::OK, answer);

}


void handle_request(http_request const &request,
                    std::function<void(json::value const &, json::value &)> action) {
    auto answer = json::value::object();
    request
            .extract_json()
            .then([&answer, &action](pplx::task<json::value> const &task) {
                try {
                    auto const &jvalue = task.get();
                    display_json(jvalue, L"R: ");
                    if (!jvalue.is_null())
                        action(jvalue, answer);

                } catch (exception const &e) {
                    wcout << e.what() << endl;
                }
            }).wait();

    display_json(answer, L"S: ");
    request.reply(status_codes::OK, answer);

}

void handle_post(http_request const &request) {
    TRACE("\nhandle POST\n");
    handle_request(request, [](json::value const &jvalue, json::value &answer) {
        for (auto const &e : jvalue.as_object()) {
            if (e.second.is_string()) {
                auto key = e.first;
                auto value = e.second;
                auto pos = dictionary.find(key);
                if (pos == dictionary.end()) {
                    answer[key] = json::value::string("Add");
                    dictionary[key] = value.as_string();
                } else {
                    answer[key] = json::value::string("<already in dict>");
                    return;
                }

            }
        }
    });
}

void handle_put(http_request const &request) {
    TRACE("\nhandle PUT\n");
    handle_request(request,
                   [](json::value const &jvalue, json::value &answer) {
                       for (auto const &e : jvalue.as_object()) {
                           if (e.second.is_string()) {
                               auto key = e.first;
                               auto value = e.second;

                               if (dictionary.find(key) == dictionary.end()) {
                                   answer[key] = json::value::string("<failed to find>");
                                   return;
                               }
                               answer[key] = json::value::string("<update>");
                               dictionary[key] = value.as_string();

                           }
                       }
                   });
}

void handle_del(http_request const &request) {
    TRACE("\nhandle DELETE\n");
    handle_request(request, []
            (json::value const &jvalue, json::value &answer) {
        set<utility::string_t> keys;
        for (auto const &e: jvalue.as_array()) {
            if (e.is_string()) {
                const auto &key = e.as_string();
                auto pos = dictionary.find(key);
                if (pos == dictionary.end()) {
                    answer[key] = json::value::string("Failed to find");

                } else {
                    answer[key] = json::value::string("deleted");
                    keys.insert(key);
                }
            }
        }
        for (auto const &key: keys)
            dictionary.erase(key);
    });
}


int main() {
    http_listener listener("http://localhost:9080");
    listener.support(methods::GET, handle_get);
    listener.support(methods::POST, handle_post);
    listener.support(methods::PUT, handle_put);
    listener.support(methods::DEL, handle_del);
    dictionary.insert(pair<utility::string_t, utility::string_t>("one", "one"));

    try {
        int num = 1;
        listener.open()
                .then([]() { TRACE("\nstarting to listen\n"); })
                .wait();
        while (num == 1)
            cin >> num;

    } catch (exception const &e) {
        wcout << e.what() << endl;
    }

    return 0;
}
