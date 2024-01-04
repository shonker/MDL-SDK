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

#ifndef MDL_DISTILLER_DIST_046_CONDITIONALS_H
#define MDL_DISTILLER_DIST_046_CONDITIONALS_H

#include "mdl_assert.h"

#include <mi/mdl/mdl_distiller_rules.h>
#include <mi/mdl/mdl_distiller_node_types.h>

namespace MI {
namespace DIST {

class Conditionals : public mi::mdl::IRule_matcher {
    public:
    virtual mi::mdl::Rule_eval_strategy get_strategy() const;
    virtual size_t get_target_material_name_count() const;
    virtual char const *get_target_material_name(size_t i) const;
    void set_node_types(mi::mdl::Node_types *node_types) {
      m_node_types = node_types;
    };

    virtual mi::mdl::DAG_node const *matcher(
        mi::mdl::IRule_matcher_event *event_handler,
        mi::mdl::IDistiller_plugin_api &engine,
        mi::mdl::DAG_node const *node,
        const mi::mdl::Distiller_options *options,
        mi::mdl::Rule_result_code &result_code) const;

    virtual bool postcond(
        mi::mdl::IRule_matcher_event *event_handler,
        mi::mdl::IDistiller_plugin_api&engine,
        mi::mdl::DAG_node const *root,
        const mi::mdl::Distiller_options *options) const;

    virtual char const * get_rule_set_name() const;

    static void fire_match_event(
        mi::mdl::IRule_matcher_event &event_handler,
        std::size_t id);

    static void fire_postcondition_event(
        mi::mdl::IRule_matcher_event &event_handler);
    

    static void fire_debug_print(
        mi::mdl::IDistiller_plugin_api &plugin_api,
        mi::mdl::IRule_matcher_event &event_handler,
        std::size_t idx,
        char const *var_name,
        mi::mdl::DAG_node const *value);
    private:
    struct Rule_info {
        unsigned ruid;
        char const *rname;
        char const *fname;
        unsigned fline;
    };

    static Rule_info const g_rule_info[1];
mi::mdl::Node_types *m_node_types = nullptr;
};


} // DIST
} // MI

#endif
// End of generated code
