// SPDX-License-Identifier: Apache-2.0
//
// NLib's answers to the Spare Kit geometry kernel's NURBS jobs.
//
// One JSON job per line on standard input, as `oracle jobs smlib-nlib` writes them; one JSON answer
// per line on standard output, every double the shortest decimal that reads back to its bits. A job
// NLib refuses answers `"values": null`. Decision 32.23 of the kernel's architecture limits this
// program to evaluation, knot insertion and removal and degree elevation, so any other job stops it.

// Before NLib's headers, whose macros (`EQ`, `GT` and the like) would break these.
#include <unistd.h>

#include <charconv>
#include <cstddef>
#include <cstdio>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

#include "json.hpp"

#include <nurbs.h>
#include <NL_Globals.h>

using nlohmann::json;

namespace {

// The tolerance of a removal job: `REMOVAL` in the kernel's `geom_test::oracle::geomdl`.
constexpr double REMOVAL = 1.0 / 1073741824.0;

double number(const json& value) {
    const std::string& text = value.get_ref<const std::string&>();
    const char* end = text.data() + text.size();
    double x = 0.0;
    const auto [stop, error] = std::from_chars(text.data(), end, x);
    if (error != std::errc() || stop != end) {
        throw std::runtime_error("not a number: " + text);
    }
    return x;
}

std::string written(double x) {
    char text[32];
    const auto [end, error] = std::to_chars(text, text + sizeof text, x);
    if (error != std::errc()) {
        throw std::runtime_error("a number that will not print");
    }
    return std::string(text, end);
}

NL_CURVE* curve_of(const json& job, NL_STACKS* stack) {
    const json& knots = job.at("knots");
    const json& points = job.at("points");
    const json& weights = job.at("weights");
    const auto last_point = static_cast<NL_INDEX>(points.size()) - 1;
    const auto last_knot = static_cast<NL_INDEX>(knots.size()) - 1;
    const auto degree = static_cast<NL_DEGREE>(job.at("degree").get<int>());
    NL_CURVE* curve = N_AllocCrvAndArrays(last_point, degree, last_knot, stack);
    if (curve == nullptr) {
        throw std::runtime_error("out of memory");
    }
    NL_CPOINT* homogeneous = nullptr;
    NL_REAL* vector = nullptr;
    N_CrvGetCPtsAndKnots(curve, &homogeneous, &vector);
    for (NL_INDEX i = 0; i <= last_knot; i++) {
        vector[i] = number(knots.at(i));
    }
    for (NL_INDEX i = 0; i <= last_point; i++) {
        const json& point = points.at(i);
        const double weight = weights.is_null() ? 1.0 : number(weights.at(i));
        homogeneous[i].x = weight * number(point.at(0));
        homogeneous[i].y = weight * number(point.at(1));
        homogeneous[i].z = point.size() > 2 ? weight * number(point.at(2)) : 0.0;
        homogeneous[i].w = weight;
    }
    return curve;
}

// The curve's knots, then its homogeneous points without their weights: the order the kernel judges.
std::vector<double> flat(NL_CURVE* curve, std::size_t dimension) {
    NL_INDEX last_point = 0;
    NL_INDEX last_knot = 0;
    NL_DEGREE degree = 0;
    NL_CPOINT* homogeneous = nullptr;
    NL_REAL* vector = nullptr;
    N_CrvGetCPtsDegreeAndKnots(curve, &last_point, &homogeneous, &degree, &last_knot, &vector);
    std::vector<double> values(vector, vector + last_knot + 1);
    for (NL_INDEX i = 0; i <= last_point; i++) {
        values.push_back(homogeneous[i].x);
        values.push_back(homogeneous[i].y);
        if (dimension > 2) {
            values.push_back(homogeneous[i].z);
        }
    }
    return values;
}

std::optional<std::vector<double>> values_of(const json& job, NL_STACKS* stack) {
    const std::string op = job.at("op").get<std::string>();
    const double at = number(job.at("at"));
    const std::size_t dimension = job.at("points").at(0).size();
    NL_CURVE* curve = curve_of(job, stack);
    N_ErrClear();
    if (op == "point") {
        NL_CPOINT at_point;
        if (N_CrvEvalPt(curve, at, NL_LEFT, &at_point) != NL_NO) {
            return std::nullopt;
        }
        std::vector<double> values{at_point.x / at_point.w, at_point.y / at_point.w};
        if (dimension > 2) {
            values.push_back(at_point.z / at_point.w);
        }
        return values;
    }
    NL_CURVE* changed = N_AllocCrv(stack);
    if (changed == nullptr) {
        throw std::runtime_error("out of memory");
    }
    N_CrvInitArrays(changed);
    NL_FLAG refused = NL_NO;
    if (op == "insert") {
        refused = N_CrvInsertKnot(curve, at, 1, changed, stack, stack);
    } else if (op == "remove") {
        NL_INDEX removed = 0;
        refused = N_CrvRemoveKnot(curve, at, 1, REMOVAL, &removed, changed, stack);
        if (removed != 1) {
            return std::nullopt;
        }
    } else if (op == "elevate") {
        refused = N_CrvElevateDegree(curve, 1, changed, stack, stack);
    } else {
        throw std::runtime_error("decision 32.23 does not cover the job " + op);
    }
    if (refused != NL_NO) {
        return std::nullopt;
    }
    return flat(changed, dimension);
}

}  // namespace

int main() {
    // Answers keep their own descriptor, so anything NLib prints lands in the log and not in them.
    const int descriptor = dup(STDOUT_FILENO);
    if (descriptor < 0 || dup2(STDERR_FILENO, STDOUT_FILENO) < 0) {
        return 2;
    }
    std::FILE* answers = fdopen(descriptor, "w");
    if (answers == nullptr) {
        return 2;
    }
    try {
        std::string line;
        while (std::getline(std::cin, line)) {
            const json job = json::parse(line);
            NL_STACKS stack;
            N_InitNurbs(&stack);
            const auto values = values_of(job, &stack);
            json answer = {{"id", job.at("id")}, {"values", nullptr}};
            if (values) {
                json texts = json::array();
                for (const double x : *values) {
                    texts.push_back(written(x));
                }
                answer["values"] = texts;
            }
            N_EndNurbs(&stack);
            std::fprintf(answers, "%s\n", answer.dump().c_str());
        }
    } catch (const std::exception& problem) {
        std::fprintf(stderr, "%s\n", problem.what());
        return 2;
    }
    return std::fclose(answers) == 0 ? 0 : 2;
}
