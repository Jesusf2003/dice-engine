#pragma once
//Minimal self-contained test helpers: no framework, the executable's exit code is the result.
//
//	RADIANT_TEST(name) { CHECK(cond); CHECK_NEAR(a, b, eps); }
//	int main() { return radiant::test::runAll(); }

#include <cmath>
#include <cstdio>
#include <functional>
#include <utility>
#include <vector>

namespace radiant::test {

struct Case
{
	const char *name;
	std::function<void()> fn;
};

inline std::vector<Case> &registry()
{
	static std::vector<Case> cases;
	return cases;
}

inline int &failureCount()
{
	static int failures = 0;
	return failures;
}

struct Registrar
{
	Registrar(const char *name, std::function<void()> fn) { registry().push_back({name, std::move(fn)}); }
};

inline void fail(const char *file, int line, const char *expr)
{
	std::printf("  FAILED %s:%d: %s\n", file, line, expr);
	++failureCount();
}

inline int runAll()
{
	int failedCases = 0;
	for (const Case &c : registry())
	{
		const int before = failureCount();
		c.fn();
		const bool ok = failureCount() == before;
		std::printf("[%s] %s\n", ok ? " OK " : "FAIL", c.name);
		failedCases += ok ? 0 : 1;
	}
	std::printf("%zu tests, %d failed\n", registry().size(), failedCases);
	return failedCases ? 1 : 0;
}

} // namespace radiant::test

#define RADIANT_TEST(name)                                                         \
	static void name();                                                            \
	static const ::radiant::test::Registrar name##_registrar(#name, &name);        \
	static void name()

#define CHECK(cond)                                                                \
	do                                                                             \
	{                                                                              \
		if (!(cond))                                                               \
			::radiant::test::fail(__FILE__, __LINE__, #cond);                      \
	} while (0)

#define CHECK_NEAR(a, b, eps) CHECK(std::fabs((a) - (b)) <= (eps))
