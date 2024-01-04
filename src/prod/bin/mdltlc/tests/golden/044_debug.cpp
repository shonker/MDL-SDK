/******************************************************************************
 * Copyright (c) 2023, NVIDIA CORPORATION. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *****************************************************************************/
// Generated by mdltlc

#include "pch.h"

#include "044_debug.h"

#include <mi/mdl/mdl_distiller_plugin_api.h>
#include <mi/mdl/mdl_distiller_plugin_helper.h>

using namespace mi::mdl;

namespace MI {
namespace DIST {

// Return the strategy to be used with this rule set.
Rule_eval_strategy Debug::get_strategy() const {
    return RULE_EVAL_BOTTOM_UP;
}

// Return the name of the rule set.
char const * Debug::get_rule_set_name() const {
    return "Debug";
}

// Return the number of imports of this rule set.
size_t Debug::get_target_material_name_count() const {
    return 0;
}

// Return the name of the import at index i.
char const *Debug::get_target_material_name(size_t i) const {
    return nullptr;
}

// Run the matcher.
DAG_node const* Debug::matcher(
    IRule_matcher_event *event_handler,
    IDistiller_plugin_api &e,
    DAG_node const *node,
    const mi::mdl::Distiller_options *options,
    Rule_result_code &result_code)const
{
    switch (e.get_selector(node)) {
    case mi::mdl::DS_DIST_BSDF_MIX_2: // match for bsdf_mix_2(_ [[ x, y ~ diffuse_reflection_bsdf() ]], _, _, bsdf())
// 044_debug.mdltl:7
//RUID 372119
        if (true
        && (e.attribute_exists(e.get_remapped_argument(node, 0), "x"))
        && (e.attribute_exists(e.get_remapped_argument(node, 0), "y")
        && (e.get_selector(e.get_attribute(e.get_remapped_argument(node, 0), "y")) == mi::mdl::IDefinition::DS_INTRINSIC_DF_DIFFUSE_REFLECTION_BSDF))
        && (e.get_selector(e.get_remapped_argument(node, 3)) == mi::mdl::DS_DIST_DEFAULT_BSDF)) {
            const DAG_node* v_matched_bsdf = node;
            const DAG_node *v_x = e.get_attribute(e.get_remapped_argument(node, 0), "x");
            const DAG_node *vv_0_y = e.get_attribute(e.get_remapped_argument(node, 0), "y");

            if (event_handler != nullptr && options != nullptr && options->debug_print)
            {
                fire_debug_print(e, *event_handler, 0, "matched_bsdf", v_matched_bsdf);
                fire_debug_print(e, *event_handler, 0, "x", v_x);
                fire_debug_print(e, *event_handler, 0, "y", v_y);
            }
            if (event_handler != nullptr)
                fire_match_event(*event_handler, 0);
            DAG_node const *node_result_1 = v_matched_bsdf;
            DAG_node const *node_result_1_x = e.create_binary(
                IDistiller_plugin_api::OK_PLUS,
                    v_x,
                    e.create_int_constant(1));
            e.set_attribute(node_result_1, "x",node_result_1_x);
            return node_result_1;
        }
// 044_debug.mdltl:9
//RUID 158827
        if (true
        && (e.get_selector(e.get_remapped_argument(node, 1)) == mi::mdl::DS_DIST_DEFAULT_BSDF)) {
            const DAG_node* v_matched_bsdf = node;
            const DAG_node* v_b = e.get_remapped_argument(node, 2);
            const DAG_node* v_c = e.get_remapped_argument(node, 3);

            if (event_handler != nullptr && options != nullptr && options->debug_print)
            {
                fire_debug_print(e, *event_handler, 1, "b", v_b);
                fire_debug_print(e, *event_handler, 1, "c", v_c);
            }
            if (event_handler != nullptr)
                fire_match_event(*event_handler, 1);
            return v_matched_bsdf;
        }
        break;
    case mi::mdl::IDefinition::DS_INTRINSIC_DF_DIFFUSE_REFLECTION_BSDF: // match for diffuse_reflection_bsdf(_, _)
// 044_debug.mdltl:12
//RUID 428539
        if (true) {
            const DAG_node* v_matched_bsdf = node;
            if (event_handler != nullptr)
                fire_match_event(*event_handler, 2);
            return v_matched_bsdf;
        }
        break;
    default:
        break;
    }

    return node;
}

bool Debug::postcond(
    IRule_matcher_event *event_handler,
    IDistiller_plugin_api &e,
    DAG_node const *root,
    const mi::mdl::Distiller_options *options) const
{
    (void)e; (void)root; // no unused variable warnings
    bool result = true;
    if (!result && event_handler != NULL)
        fire_postcondition_event(*event_handler);
    return result;
}

void Debug::fire_match_event(
    mi::mdl::IRule_matcher_event &event_handler,
    std::size_t id)
{
    Rule_info const &ri = g_rule_info[id];
    event_handler.rule_match_event("Debug", ri.ruid, ri.rname, ri.fname, ri.fline);
}

void Debug::fire_postcondition_event(
mi::mdl::IRule_matcher_event &event_handler)
{
    event_handler.postcondition_failed("Debug");
}

void Debug::fire_debug_print(
    mi::mdl::IDistiller_plugin_api &plugin_api,
    mi::mdl::IRule_matcher_event &event_handler,
    std::size_t idx,
    char const *var_name,
    DAG_node const *value)
{
    Rule_info const &ri = g_rule_info[idx];
    event_handler.debug_print(plugin_api, "Debug", ri.ruid, ri.rname, ri.fname, ri.fline,
        var_name, value);
}


// Rule info table.
Debug::Rule_info const Debug::g_rule_info[3] = {
    { 372119, "bsdf_mix_2", "044_debug.mdltl", 7 },
    { 158827, "bsdf_mix_2", "044_debug.mdltl", 9 },
    { 428539, "matched", "044_debug.mdltl", 12 }
};


} // DIST
} // MI
// End of generated code
