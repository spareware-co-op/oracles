// Manifold's answers to the Spare Kit geometry kernel's mesh booleans.
//
// One JSON job per line on standard input, as `oracle jobs manifold` writes them; one JSON answer
// per line on standard output. Corners are whole grid steps, which a double holds exactly, and
// every double is written to read back to the same bits. A job Manifold refuses answers
// `"volume": null`.

#include <cstdint>
#include <cstdio>
#include <iostream>
#include <string>

#include <manifold/manifold.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using manifold::Manifold;
using manifold::MeshGL64;
using manifold::OpType;

static Manifold solid(const json& s) {
  MeshGL64 mesh;
  mesh.numProp = 3;
  for (const auto& corner : s.at("vertices")) {
    for (const auto& x : corner) mesh.vertProperties.push_back(x.get<double>());
  }
  for (const auto& triangle : s.at("triangles")) {
    for (const auto& v : triangle) mesh.triVerts.push_back(v.get<std::uint64_t>());
  }
  return Manifold(mesh);
}

static std::string text(double x) {
  char written[40];
  std::snprintf(written, sizeof written, "%.17g", x);
  return written;
}

static OpType op_of(const std::string& op) {
  if (op == "union") return OpType::Add;
  if (op == "intersection") return OpType::Intersect;
  return OpType::Subtract;
}

int main() {
  std::string line;
  while (std::getline(std::cin, line)) {
    const json job = json::parse(line);
    json answer = {{"id", job.at("id")}, {"volume", nullptr}};
    const Manifold made = solid(job.at("a")).Boolean(solid(job.at("b")),
                                                     op_of(job.at("op").get<std::string>()));
    if (made.Status() == Manifold::Error::NoError) {
      const MeshGL64 mesh = made.GetMeshGL64();
      json triangles = json::array();
      const std::size_t stride = mesh.numProp;
      for (std::size_t t = 0; t + 2 < mesh.triVerts.size(); t += 3) {
        json triangle = json::array();
        for (std::size_t k = 0; k < 3; ++k) {
          const std::size_t at = mesh.triVerts[t + k] * stride;
          triangle.push_back({text(mesh.vertProperties[at]), text(mesh.vertProperties[at + 1]),
                              text(mesh.vertProperties[at + 2])});
        }
        triangles.push_back(triangle);
      }
      answer["volume"] = text(made.Volume());
      answer["area"] = text(made.SurfaceArea());
      answer["tolerance"] = text(made.GetTolerance());
      answer["triangles"] = triangles;
    }
    std::cout << answer.dump() << '\n';
  }
  return 0;
}
