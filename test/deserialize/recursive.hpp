#include "common.hpp"
#include "dynser/dynser.h"

TEST_CASE("Recursive")
{
    using namespace dynser_test;

    dynser::DynSer ser{
        dynser::generate_property_to_target_mapper(
            [&](dynser::Context& ctx, dynser::Properties const& props, std::vector<int>& out) {
                if (props.empty()) {
                    out = {};
                    return;
                }
                auto props_tmp = props;
                auto last_element = props_tmp.at("last-element").as_const_i32();
                while (props_tmp.contains("element")) {
                    out.push_back(props_tmp.at("element").as_const_i32());
                    props_tmp = dynser::util::remove_prefix(props_tmp, "next");
                }
                out.push_back(last_element);
            }
        ),
        dynser::generate_property_to_target_mapper(),
    };

    DYNSER_LOAD_CONFIG_FILE(ser, "recursive.yaml");

    SECTION("'recursive' rule", "[continual] [recursive] [existing] [linear] [required]")
    {
        using Target = std::vector<int>;
        TestCases<Target> recursives{
            { "[ 1, 2, 3 ]", { 1, 2, 3 } },
            { "[ ]", {} },
            { "[ -1, -3, 10, 0, 9999 ]", { -1, -3, 10, 0, 9999 } },
            { "[ 0, 12, 12, 12, 12, 12, 12, 13, 0, 0, 0 ]", { 0, 12, 12, 12, 12, 12, 12, 13, 0, 0, 0 } },
            { "[ 1 ]", { 1 } },
            { "[ 12 ]", { 12 } },
            { "[ 13, -21 ]", { 13, -21 } },
            { "[ -10 ]", { -10 } },
        };

        DYNSER_TEST_FOREACH_DESERIALIZE(Target, recursives, "recursive", "Recursive #");
    }
}
