#include "httplib.h"
#include "json.hpp"
#include "create_map.cpp"
#include <iostream>
#include <vector>
#include <algorithm>
#include <queue>

using json = nlohmann::json;

Graph<std::string> g_str;
Graph<int> g_int;

void init_server_data() {
    campus_map_init(campus_map_str_int);
    campus_map_init(campus_map_int_str);
    g_str = campus_map_create_string();
    g_int = campus_map_create_int();
}

// 核心修复：获取当前最大的 ID，防止节点删除后数组越界
int get_max_id() {
    int max_id = 0;
    for (auto const& pair : campus_map_int_str) max_id = std::max(max_id, pair.first);
    return max_id + 1;
}

// 核心修复：鲁棒的自定义 Floyd 算法，免疫 ID 间断和越界
void robust_floyd(Graph<int>& graph, std::vector<std::vector<int>>& dist, std::vector<std::vector<int>>& path, int MAX) {
    for (int i = 0; i < MAX; i++) {
        for (int j = 0; j < MAX; j++) {
            dist[i][j] = (i == j) ? 0 : INF;
            path[i][j] = -1;
        }
    }
    for (const auto& u : graph.adj) {
        for (const auto& v : u.second) dist[u.first][v.vertex] = v.weight;
    }
    for (int k = 0; k < MAX; k++) {
        for (int i = 0; i < MAX; i++) {
            for (int j = 0; j < MAX; j++) {
                if (dist[i][k] < INF && dist[k][j] < INF) {
                    if (dist[i][j] > dist[i][k] + dist[k][j]) {
                        dist[i][j] = dist[i][k] + dist[k][j];
                        path[i][j] = k;
                    }
                }
            }
        }
    }
}

// 辅助算法
void dfs_op5(std::vector<std::vector<int>> &graph, int src, int dest, std::vector<int> &path, std::vector<bool> &visited, int path_len, int n, std::vector<int> &shortest_path, int &shortest_dis) {
    if (path_len >= shortest_dis) return; // 剪枝
    visited[src] = true; path.push_back(src);
    if (src == dest && path.size() == n) {
        if (path_len < shortest_dis) { shortest_path = path; shortest_dis = path_len; }
    } else {
        for (int i = 0; i < graph[src].size(); i++) {
            if (!visited[i] && graph[src][i] < INF && graph[src][i] > 0)
                dfs_op5(graph, i, dest, path, visited, path_len + graph[src][i], n, shortest_path, shortest_dis);
        }
    }
    path.pop_back(); visited[src] = false;
}

void get_path(int a, int b, std::vector<std::vector<int>> &path, std::vector<int> &anspath) {
    if (path[a][b] == -1) return;
    int k = path[a][b];
    get_path(a, k, path, anspath);
    anspath.push_back(k);
    get_path(k, b, path, anspath);
}

int get_dis(int src, int dest, std::vector<std::vector<int>> &dist, std::vector<int> &must) {
    if (must.empty()) return dist[src][dest];
    int distance = dist[src][must[0]];
    if (distance >= INF) return INF;
    for (size_t i = 0; i < must.size() - 1; i++) {
        if (dist[must[i]][must[i + 1]] >= INF) return INF;
        distance += dist[must[i]][must[i + 1]];
    }
    if (dist[must.back()][dest] >= INF) return INF;
    distance += dist[must.back()][dest];
    return distance;
}

void save_path(int src, int dest, std::vector<int> &must, std::vector<int> &anspath, std::vector<std::vector<int>> &path) {
    anspath.push_back(src);
    if (must.empty()) { get_path(src, dest, path, anspath); } 
    else {
        get_path(src, must[0], path, anspath);
        for (size_t i = 0; i < must.size() - 1; i++) {
            anspath.push_back(must[i]); get_path(must[i], must[i + 1], path, anspath);
        }
        anspath.push_back(must.back()); get_path(must.back(), dest, path, anspath);
    }
    anspath.push_back(dest);
}

bool is_cyclic(std::vector<std::vector<int>> &graph) {
    int n = graph.size(); std::vector<int> in_degree(n, 0);
    for (int i = 0; i < n; i++) for (int j = 0; j < n; j++) if (graph[i][j]) in_degree[j]++;
    std::queue<int> q;
    for (int i = 0; i < n; i++) if (in_degree[i] == 0) q.push(i);
    int cnt = 0;
    while (!q.empty()) {
        int u = q.front(); q.pop();
        for (int i = 0; i < n; i++) if (graph[u][i] && --in_degree[i] == 0) q.push(i);
        cnt++;
    }
    return cnt != n;
}

int main() {
    init_server_data();
    httplib::Server svr;

    svr.set_post_routing_handler([](const auto& req, auto& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
    });
    svr.Options("/(.*)", [](const auto& req, auto& res) { res.status = 200; });

    svr.Get("/api/map", [&](const httplib::Request& req, httplib::Response& res) {
        json response; response["nodes"] = json::array(); response["edges"] = json::array();
        for (const auto& u : g_str.adj) {
            response["nodes"].push_back({ {"id", u.first}, {"name", u.first} });
            for (const auto& v : u.second) response["edges"].push_back({{"source", u.first}, {"target", v.vertex}, {"weight", v.weight}});
        }
        res.set_content(response.dump(), "application/json");
    });

    svr.Post("/api/edit", [&](const httplib::Request& req, httplib::Response& res) {
        json response;
        try {
            auto body = json::parse(req.body);
            std::string type = body["type"], action = body["action"];
            if (type == "node") {
                std::string node = body["node"];
                if (action == "add" && !g_str.contains(node)) {
                    int vnum = ++BASE; campus_map_str_int[node] = vnum; campus_map_int_str[vnum] = node;
                    g_str.add_vertex(node); g_int.add_vertex(vnum); response["status"] = "success";
                } else if (action == "delete" && g_str.contains(node)) {
                    int vnum = campus_map_str_int[node];
                    campus_map_str_int.erase(node); campus_map_int_str.erase(vnum);
                    g_str.remove_vertex(node); g_int.remove_vertex(vnum); response["status"] = "success";
                } else response["error"] = "节点操作无效";
            } else if (type == "edge") {
                std::string src = body["source"], dest = body["target"];
                if (action == "add") {
                    int weight = body["weight"];
                    if (!g_str.contains(src)) { int v = ++BASE; campus_map_str_int[src]=v; campus_map_int_str[v]=src; g_str.add_vertex(src); g_int.add_vertex(v); }
                    if (!g_str.contains(dest)) { int v = ++BASE; campus_map_str_int[dest]=v; campus_map_int_str[v]=dest; g_str.add_vertex(dest); g_int.add_vertex(v); }
                    g_str.add_edge(src, dest, weight); g_int.add_edge(campus_map_str_int[src], campus_map_str_int[dest], weight);
                    response["status"] = "success";
                } else if (action == "delete") {
                    if (g_str.adj[src].size() > 1 && g_str.adj[dest].size() > 1) {
                        g_str.remove_edge(src, dest); g_int.remove_edge(campus_map_str_int[src], campus_map_str_int[dest]);
                        response["status"] = "success";
                    } else response["error"] = "删除导致拓扑孤岛，拒绝操作";
                }
            }
        } catch (...) { response["error"] = "参数解析失败"; }
        res.set_content(response.dump(), "application/json");
    });

    svr.Post("/api/reset", [&](const httplib::Request& req, httplib::Response& res) {
        campus_map_str_int.clear();
        campus_map_int_str.clear();
        
        // 重置自增 ID 基数 (定义在 create_map.cpp 中)
        BASE = 14; 

        g_str = Graph<std::string>(); 
        g_int = Graph<int>();
        init_server_data(); 
        
        res.set_content(R"({"status": "success"})", "application/json");
    });

    svr.Post("/api/path/shortest", [&](const httplib::Request& req, httplib::Response& res) {
        auto body = json::parse(req.body);
        if (!g_str.contains(body["source"]) || !g_str.contains(body["target"])) { res.set_content(R"({"error": "顶点异常"})", "application/json"); return; }
        auto result = g_str.dijkstra(body["source"], body["target"]);
        json response = {{"distance", result.first}, {"path", result.second}};
        if (result.first >= INF) response["error"] = "路由不连通";
        res.set_content(response.dump(), "application/json");
    });

    svr.Post("/api/path/n_nodes", [&](const httplib::Request& req, httplib::Response& res) {
        auto body = json::parse(req.body);
        std::string src_str = body["source"], dest_str = body["target"]; int n = body["n"];
        int MAX = get_max_id();
        std::vector<std::vector<int>> dist(MAX, std::vector<int>(MAX, INF)), path(MAX, std::vector<int>(MAX, -1));
        robust_floyd(g_int, dist, path, MAX); // 利用鲁棒生成底图
        
        int src = campus_map_str_int[src_str], dest = campus_map_str_int[dest_str];
        std::vector<int> dfs_path; std::vector<bool> visited(MAX, false);
        int path_len = 0, shortest_dis = INF; std::vector<int> shortest_path;
        
        dfs_op5(dist, src, dest, dfs_path, visited, path_len, n, shortest_path, shortest_dis);
        
        json response;
        if (shortest_dis != INF && !shortest_path.empty()) {
            response["distance"] = shortest_dis;
            std::vector<std::string> sp; for (int node : shortest_path) sp.push_back(campus_map_int_str[node]);
            response["path"] = sp;
        } else response["error"] = "无法满足约束：不存在该定长顶点的连通图";
        res.set_content(response.dump(), "application/json");
    });

    svr.Post("/api/path/tour", [&](const httplib::Request& req, httplib::Response& res) {
        json response; auto body = json::parse(req.body); std::vector<std::string> places = body["places"];
        if (places.size() < 2) { response["error"] = "TSP 约束需至少 2 个顶点"; }
        else {
            int MAX = get_max_id();
            std::vector<int> key_place; for (auto& p : places) key_place.push_back(campus_map_str_int[p]);
            sort(key_place.begin(), key_place.end());
            
            // 安全提取根节点进行 Prim
            int root = campus_map_int_str.begin()->first;
            Graph<int> mst_int = g_int.prim(root);
            std::vector<std::vector<int>> dist(MAX, std::vector<int>(MAX, INF)), path(MAX, std::vector<int>(MAX, -1));
            robust_floyd(mst_int, dist, path, MAX); // 生成生成树的最短路径矩阵
            
            int shortest_dis = INF; std::vector<int> anspath; int k = key_place.size();
            do {
                int src = key_place[0], dest = key_place[k - 1];
                std::vector<int> must(key_place.begin() + 1, key_place.end() - 1);
                int dis = get_dis(src, dest, dist, must);
                if (dis < shortest_dis) { shortest_dis = dis; anspath.clear(); save_path(src, dest, must, anspath, path); }
            } while (next_permutation(key_place.begin(), key_place.end()));
            
            if (shortest_dis >= INF) response["error"] = "生成树范围内节点不连通";
            else {
                response["distance"] = shortest_dis; std::vector<std::string> sp;
                for (int node : anspath) sp.push_back(campus_map_int_str[node]); response["path"] = sp;
            }
        }
        res.set_content(response.dump(), "application/json");
    });

    // 彻底重构的 Op7 受限顺序遍历
    svr.Post("/api/path/restricted", [&](const httplib::Request& req, httplib::Response& res) {
        json response; auto body = json::parse(req.body);
        std::vector<std::string> places = body["places"]; auto limits_json = body["limits"];
        if (places.size() < 2) { response["error"] = "请先指定起点和终点"; res.set_content(response.dump(), "application/json"); return; }
        
        int MAX = get_max_id();
        int src = campus_map_str_int[places.front()], dest = campus_map_str_int[places.back()];
        
        std::vector<std::pair<int, int>> lim; std::vector<std::vector<int>> lim_edges(MAX, std::vector<int>(MAX, 0));
        for (auto& l : limits_json) {
            int u = campus_map_str_int[l["from"]], v = campus_map_str_int[l["to"]];
            lim_edges[u][v] = 1; lim.push_back({u, v});
        }
        if (is_cyclic(lim_edges)) { response["error"] = "前置依赖构成死锁环路"; res.set_content(response.dump(), "application/json"); return; }
        
        std::vector<std::vector<int>> dist(MAX, std::vector<int>(MAX, INF)), path(MAX, std::vector<int>(MAX, -1));
        robust_floyd(g_int, dist, path, MAX);

        std::vector<int> mid_nodes;
        for (size_t i = 1; i < places.size() - 1; i++) mid_nodes.push_back(campus_map_str_int[places[i]]);
        sort(mid_nodes.begin(), mid_nodes.end());

        int shortest_dis = INF; std::vector<int> anspath;
        do {
            bool valid = true;
            std::vector<int> seq = {src}; seq.insert(seq.end(), mid_nodes.begin(), mid_nodes.end()); seq.push_back(dest);
            for (auto& l : lim) {
                auto it1 = std::find(seq.begin(), seq.end(), l.first);
                auto it2 = std::find(seq.begin(), seq.end(), l.second);
                if (it1 != seq.end() && it2 != seq.end() && std::distance(seq.begin(), it1) > std::distance(seq.begin(), it2)) { valid = false; break; }
            }
            if (!valid) continue;

            int dis = get_dis(src, dest, dist, mid_nodes);
            if (dis < shortest_dis) { shortest_dis = dis; anspath.clear(); save_path(src, dest, mid_nodes, anspath, path); }
        } while (next_permutation(mid_nodes.begin(), mid_nodes.end()));

        if (shortest_dis >= INF) response["error"] = "受限条件导致拓扑不可达";
        else {
            response["distance"] = shortest_dis; std::vector<std::string> sp;
            for (auto node : anspath) sp.push_back(campus_map_int_str[node]); response["path"] = sp;
        }
        res.set_content(response.dump(), "application/json");
    });

    svr.listen("0.0.0.0", 8080);
    return 0;
}