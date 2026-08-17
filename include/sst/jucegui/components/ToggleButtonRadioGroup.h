/*
 * sst-jucegui - an open source library of juce widgets
 * built by Surge Synth Team.
 *
 * Copyright 2023-2024, various authors, as described in the GitHub
 * transaction log.
 *
 * sst-jucegui is released under the MIT license, as described
 * by "LICENSE.md" in this repository. This means you may use this
 * in commercial software if you are a JUCE Licensee. If you use JUCE
 * in the open source / GPL3 context, your combined work must be
 * released under GPL3.
 *
 * All source in sst-jucegui available at
 * https://github.com/surge-synthesizer/sst-jucegui
 */

#ifndef INCLUDE_SST_JUCEGUI_COMPONENTS_TOGGLEBUTTONRADIOGROUP_H
#define INCLUDE_SST_JUCEGUI_COMPONENTS_TOGGLEBUTTONRADIOGROUP_H

#include <juce_gui_basics/juce_gui_basics.h>

#include <algorithm>
#include <memory>
#include <sst/jucegui/style/StyleAndSettingsConsumer.h>
#include <sst/jucegui/style/StyleSheet.h>
#include <sst/jucegui/data/Discrete.h>
#include <sst/jucegui/components/BaseStyles.h>
#include <sst/jucegui/util/WheelCalibration.h>

#include <string>

#include "ComponentBase.h"
#include "ToggleButton.h"

namespace sst::jucegui::components
{
struct ToggleButtonRadioGroup : public juce::Component,
                                public style::SettingsConsumer,
                                public EditableComponentBase<ToggleButtonRadioGroup>,
                                public data::Discrete::DataListener
{
    ToggleButtonRadioGroup();
    ~ToggleButtonRadioGroup();

    void dataChanged() override;
    void setSource(data::Discrete *d)
    {
        if (data)
            data->removeGUIDataListener(this);
        data = d;
        if (data)
            data->addGUIDataListener(this);
        dataChanged();
        repaint();
    }

    void sourceVanished(data::Discrete *d) override
    {
        data->removeGUIDataListener(this);
        data = nullptr;
        repaint();
    }

    void resized() override;

    /*
     * The wheel walks the selection, up for the previous entry, matching the
     * arrow keys and the other list-style widgets.
     *
     * This deliberately clamps rather than wrapping. A radio group is a row of
     * tabs, and spinning off the end of a short row back round to the start
     * makes it hard to land on the last one.
     */
    void mouseWheelMove(const juce::MouseEvent &e, const juce::MouseWheelDetails &wheel) override
    {
        if constexpr (util::onMac)
        {
            juce::Component::mouseWheelMove(e, wheel);
        }
        else
        {
            if (!data)
                return;

            auto steps = wheelDetentAcc(e, wheel);
            if (steps == 0)
                return;

            auto v = std::clamp(data->getValue() - steps, data->getMin(), data->getMax());
            if (v != data->getValue())
                data->setValueFromGUI(v);
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ToggleButtonRadioGroup);

  private:
    std::string label;
    data::Discrete *data{nullptr};
    util::DetentAccumulator wheelDetentAcc;

    std::vector<std::unique_ptr<ToggleButton>> buttons;
    std::vector<std::unique_ptr<data::Discrete>> buttonSubData;
};
} // namespace sst::jucegui::components
#endif // SHORTCIRCUITXT_TOGGLEBUTTONRADIOGROUP_H
