#include <unordered_map>

#include <catch2/catch2.hpp>

#if CHOWDSP_WDF_TEST_WITH_XSIMD
#include <xsimd/xsimd.hpp>
#endif

#include <chowdsp_wdf/chowdsp_wdf.h>

/** Reference values generated from scipy.special */
std::unordered_map<double, double> WO_vals {
    { -10.0, 4.539786874921544e-05 },
    { -9.5, 7.484622772024869e-05 },
    { -9.0, 0.00012339457692560975 },
    { -8.5, 0.00020342698226408345 },
    { -8.0, 0.000335350149321062 },
    { -7.5, 0.0005527787213627528 },
    { -7.0, 0.0009110515723789146 },
    { -6.5, 0.0015011839473879653 },
    { -6.0, 0.002472630709097278 },
    { -5.5, 0.004070171383753891 },
    { -5.0, 0.0066930004977309955 },
    { -4.5, 0.010987603420879434 },
    { -4.0, 0.017989102828531025 },
    { -3.5, 0.029324711813756815 },
    { -3.0, 0.04747849102486547 },
    { -2.5, 0.07607221340790257 },
    { -2.0, 0.1200282389876412 },
    { -1.5, 0.1853749184489398 },
    { -1.0, 0.27846454276107374 },
    { -0.5, 0.4046738485459385 },
    { 0.0, 0.5671432904097838 },
    { 0.5, 0.7662486081617502 },
    { 1.0, 1.0 },
    { 1.5, 1.2649597201255005 },
    { 2.0, 1.5571455989976113 },
    { 2.5, 1.8726470404165942 },
    { 3.0, 2.207940031569323 },
    { 3.5, 2.559994780412122 },
    { 4.0, 2.926271062443501 },
    { 4.5, 3.3046649181693253 },
    { 5.0, 3.6934413589606496 },
    { 5.5, 4.091169202271799 },
    { 6.0, 4.4966641730061605 },
    { 6.5, 4.908941634486258 },
    { 7.0, 5.327178301371093 },
    { 7.5, 5.750681611147114 },
    { 8.0, 6.178865346308128 },
    { 8.5, 6.611230244734983 },
    { 9.0, 7.047348546597604 },
    { 9.5, 7.486851633496902 },
    { 10.0, 7.9294200950196965 },
};

#if CHOWDSP_WDF_TEST_WITH_XSIMD
template <typename T>
struct SIMDApproxImpl
{
    explicit SIMDApproxImpl (xsimd::batch<T> const& value) : m_value (value)
    {
    }

    SIMDApproxImpl& margin (T const& newMargin)
    {
        m_margin = newMargin;
        return *this;
    }

    friend bool operator== (const xsimd::batch<T>& lhs, SIMDApproxImpl const& rhs)
    {
        bool result = true;
        for (size_t i = 0; i < xsimd::batch<T>::size; ++i)
            result &= (lhs.get (i) == Approx (rhs.m_value.get (0)).margin (rhs.m_margin));

        return result;
    }

private:
    xsimd::batch<T> m_value;
    T m_margin;
};

template <typename T>
using SIMDApprox = typename std::conditional<! std::is_floating_point<std::remove_const_t<T>>::value, SIMDApproxImpl<chowdsp::NumericType<T>>, Approx>::type;
#else
template <typename T>
using SIMDApprox = Approx;
#endif

template <typename T>
using FuncType = std::function<T (T)>;

template <typename T>
struct FunctionTest
{
    chowdsp::NumericType<T> low;
    chowdsp::NumericType<T> high;
    FuncType<T> testFunc;
    FuncType<chowdsp::NumericType<T>> refFunc;
    chowdsp::NumericType<T> tol;
};

template <typename T>
void checkFunctionAccuracy (const FunctionTest<T>& funcTest, size_t N = 20)
{
    auto step = (funcTest.high - funcTest.low) / (chowdsp::NumericType<T>) N;
    for (chowdsp::NumericType<T> x = funcTest.low; x < funcTest.high; x += step)
    {
        auto expected = funcTest.refFunc (x);
        auto actual = funcTest.testFunc (x);

        REQUIRE (actual == SIMDApprox<T> (expected).margin (funcTest.tol));
    }
}

template <typename T, typename Func>
void checkWrightOmega (Func&& omega, chowdsp::NumericType<T> tol)
{
    for (auto vals : WO_vals)
    {
        auto expected = (T) (chowdsp::NumericType<T>) vals.second;
        auto actual = omega ((T) (chowdsp::NumericType<T>) vals.first);

        REQUIRE (actual == SIMDApprox<T> (expected).margin (tol));
    }
}

#if CHOWDSP_WDF_TEST_WITH_XSIMD
TEST_CASE ("SIMD Cast Test")
{
    SECTION ("Float to Int32")
    {
        xsimd::batch<float> x { -23.1f, 0.0f, 33.22f, 101.2f };
        const auto int_batch = chowdsp::xsimd_cast<int32_t> (x);
        REQUIRE (int_batch.get (0) == -23);
        REQUIRE (int_batch.get (1) == 0);
        REQUIRE (int_batch.get (2) == 33);
        REQUIRE (int_batch.get (3) == 101);
    }

    SECTION ("Int32 to Float")
    {
        xsimd::batch<int32_t> x { -23, 0, 33, 101 };
        const auto float_batch = chowdsp::xsimd_cast<float> (x);
        REQUIRE (float_batch.get (0) == -23.0f);
        REQUIRE (float_batch.get (1) == 0.0f);
        REQUIRE (float_batch.get (2) == 33.0f);
        REQUIRE (float_batch.get (3) == 101.0f);
    }

    SECTION ("Double to Int64")
    {
        {
            xsimd::batch<double> x { -23.1, 0.0 };
            const auto int_batch = chowdsp::xsimd_cast<int64_t> (x);
            REQUIRE (int_batch.get (0) == -23);
            REQUIRE (int_batch.get (1) == 0);
        }
        {
            xsimd::batch<double> x { 33.22, 101.2 };
            const auto int_batch = chowdsp::xsimd_cast<int64_t> (x);
            REQUIRE (int_batch.get (0) == 33);
            REQUIRE (int_batch.get (1) == 101);
        }
    }

    SECTION ("Int64 to Double")
    {
        {
            xsimd::batch<int64_t> x { -23, 0 };
            const auto double_batch = chowdsp::xsimd_cast<double> (x);
            REQUIRE (double_batch.get (0) == -23.0);
            REQUIRE (double_batch.get (1) == 0.0);
        }
        {
            xsimd::batch<int64_t> x { 33, 101 };
            const auto double_batch = chowdsp::xsimd_cast<double> (x);
            REQUIRE (double_batch.get (0) == 33.0);
            REQUIRE (double_batch.get (1) == 101.0);
        }
    }
}
#endif

#if CHOWDSP_WDF_TEST_WITH_XSIMD
TEMPLATE_TEST_CASE ("Omega Test", "", float, double, xsimd::batch<float>, xsimd::batch<double>)
#else
TEMPLATE_TEST_CASE ("Omega Test", "", float, double)
#endif
{
    SECTION ("Log2 Test")
    {
        checkFunctionAccuracy (FunctionTest<TestType> {
            1.0f,
            2.0f,
            [] (auto x) { return chowdsp::Omega::log2_approx<chowdsp::NumericType<TestType>> (x); },
            [] (auto x) { return std::log2 (x); },
            0.008f });
    }

    SECTION ("Log Test")
    {
        checkFunctionAccuracy (FunctionTest<TestType> {
            8.0f,
            12.0f,
            [] (auto x) { return chowdsp::Omega::log_approx<TestType> (x); },
            [] (auto x) { return std::log (x); },
            0.005f });
    }

    SECTION ("Pow2 Test")
    {
        checkFunctionAccuracy (FunctionTest<TestType> {
            0.0f,
            1.0f,
            [] (auto x) { return chowdsp::Omega::pow2_approx<chowdsp::NumericType<TestType>> (x); },
            [] (auto x) { return std::pow (2.0f, x); },
            0.001f });
    }

    SECTION ("Exp Test")
    {
        checkFunctionAccuracy (FunctionTest<TestType> {
            -4.0f,
            2.0f,
            [] (auto x) { return chowdsp::Omega::exp_approx<TestType> (x); },
            [] (auto x) { return std::exp (x); },
            0.03f });
    }

    SECTION ("Omega1 Test")
    {
        checkWrightOmega<TestType> ([] (TestType x) { return chowdsp::Omega::omega1 (x); },
                                    2.1f);
    }

    SECTION ("Omega2 Test")
    {
        checkWrightOmega<TestType> ([] (TestType x) { return chowdsp::Omega::omega2 (x); },
                                    2.1f);
    }

    SECTION ("Omega3 Test")
    {
        checkWrightOmega<TestType> ([] (TestType x) { return chowdsp::Omega::omega3 (x); },
                                    0.3f);
    }

    SECTION ("Omega4 Test")
    {
        checkWrightOmega<TestType> ([] (TestType x) { return chowdsp::Omega::omega4 (x); },
                                    0.05f);
    }
}
