/*
 * sst-juce-gui - an open source library of juce widgets
 * built by Surge Synth Team.
 *
 * Copyright 2023, various authors, as described in the GitHub
 * transaction log.
 *
 * sst-basic-blocks is released under the MIT license, as described
 * by "LICENSE.md" in this repository. This means you may use this
 * in commercial software if you are a JUCE Licensee. If you use JUCE
 * in the open source / GPL3 context, your combined work must be
 * released under GPL3.
 *
 * All source in sst-juce-gui available at
 * https://github.com/surge-synthesizer/sst-juce-gui
 */

#ifndef INCLUDE_SST_JUCEGUI_COMPONENTS_CONTINUOUSPARAMEDITOR_H
#define INCLUDE_SST_JUCEGUI_COMPONENTS_CONTINUOUSPARAMEDITOR_H

#include <string>

#include <juce_gui_basics/juce_gui_basics.h>

#include <sst/jucegui/style/StyleAndSettingsConsumer.h>
#include <sst/jucegui/style/StyleSheet.h>

#include "ComponentBase.h"
#include "BaseStyles.h"

namespace sst::jucegui::components
{
struct ContinuousParamEditor : public juce::Component,
                               public Modulatable<ContinuousParamEditor>,
                               public EditableComponentBase<ContinuousParamEditor>,
                               public style::SettingsConsumer
{
    struct Styles : base_styles::ValueBearing,
                    base_styles::ModulationValueBearing,
                    base_styles::GraphicalHandle,
                    base_styles::ValueGutter,
                    base_styles::BaseLabel,
                    base_styles::Base,
                    base_styles::Outlined
    {
        SCLASS(continuousparameditor);

        static void initialize()
        {
            style::StyleSheet::addClass(styleClass)
                .withBaseClass(base_styles::ValueBearing::styleClass)
                .withBaseClass(base_styles::ModulationValueBearing::styleClass)
                .withBaseClass(base_styles::GraphicalHandle::styleClass)
                .withBaseClass(base_styles::BaseLabel::styleClass)
                .withBaseClass(base_styles::Base::styleClass)
                .withBaseClass(base_styles::Outlined::styleClass)
                .withBaseClass(base_styles::ValueGutter::styleClass);
        }
    };

    enum Direction
    {
        VERTICAL,
        HORIZONTAL
    };
    ContinuousParamEditor(Direction primaryDirection);
    ~ContinuousParamEditor();

    void paint(juce::Graphics &g) override { g.fillAll(juce::Colours::red); }

    // Various enablement and hidden conditions get put here
    bool processMouseActions();

    // and then queried in these events
    void mouseDown(const juce::MouseEvent &e) override;
    void mouseUp(const juce::MouseEvent &e) override;
    void mouseDrag(const juce::MouseEvent &e) override;
    void mouseDoubleClick(const juce::MouseEvent &e) override;
    void mouseWheelMove(const juce::MouseEvent &event,
                        const juce::MouseWheelDetails &wheel) override;

    void mouseEnter(const juce::MouseEvent &e) override { startHover(); }
    void mouseExit(const juce::MouseEvent &e) override { endHover(); }
    void mouseMove(const juce::MouseEvent &e) override { resetTimer(e); }

  protected:
    float mouseDownV0, mouseDownX0, mouseDownY0;

    enum MouseMode
    {
        NONE,
        POPUP,
        DRAG
    } mouseMode{NONE};
    Direction direction;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ContinuousParamEditor)
};
} // namespace sst::jucegui::components

#endif // SST_JUCEGUI_CONTINUOUSPARAMEDITOR_H
