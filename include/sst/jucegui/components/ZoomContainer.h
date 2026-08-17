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

#ifndef INCLUDE_SST_JUCEGUI_COMPONENTS_ZOOMCONTAINER_H
#define INCLUDE_SST_JUCEGUI_COMPONENTS_ZOOMCONTAINER_H

#include <algorithm>
#include <memory>
#include <juce_gui_basics/juce_gui_basics.h>
#include <sst/jucegui/util/WheelCalibration.h>
#include "ScrollBar.h"
#include "ToolTip.h"

namespace sst::jucegui::components
{
struct ZoomContainerClient
{
    virtual ~ZoomContainerClient() = default;
    virtual juce::Component *associatedComponent() = 0;
    virtual bool supportsVerticalZoom() const = 0;
    virtual bool supportsHorizontalZoom() const = 0;
    virtual void setHorizontalZoom(float pctStart, float zoomFactor) {}
    virtual void setVerticalZoom(float pctStart, float zoomFactor) {}
};

struct ZoomContainer : juce::Component, juce::ScrollBar::Listener
{
    struct MagControl : juce::Component
    {
        MagControl(ZoomContainer *z, ScrollBar *v, ScrollBar *h) : z(z), v(v), h(h)
        {
            if (!(h || v))
                return;
            toolTip = std::make_unique<ToolTip>();
            z->addChildComponent(*toolTip);
        }

        void activateTooltip(bool show)
        {
            if (show && toolTip && ((h && h->style()) || (v && v->style())))
            {
                if (h)
                    toolTip->setStyle(h->style());
                else
                    toolTip->setStyle(v->style());
                toolTip->setTooltipTitleAndData("Drag to zoom", std::vector<std::string>());

                auto sz = toolTip->getBounds().expanded(2);
                auto bc =
                    z->getLocalBounds().getBottomRight() -
                    juce::Point<int>(sz.getWidth() + getWidth(), sz.getHeight() + getHeight());
                toolTip->setBounds(bc.getX(), bc.getY(), sz.getWidth(), sz.getHeight());
                toolTip->setVisible(true);
            }
            else
            {
                if (toolTip)
                    toolTip->setVisible(false);
            }
        }

        void paint(juce::Graphics &g) override
        {
            if (!(h || v) || !z)
                return;

            const auto &style = (h ? h->style() : v->style());
            auto c = style->getColour(ScrollBar::Styles::styleClass, ScrollBar::Styles::outline);
            g.setColour(c);
            auto b = getLocalBounds().withTrimmedRight(2).withTrimmedBottom(2).reduced(1);
            g.drawEllipse(b.toFloat(), 2);
            auto fp = getLocalBounds().getBottomRight();
            auto cp = fp - juce::Point<int>(3, 3);
            g.drawLine(fp.getX(), fp.getY(), cp.getX(), cp.getY(), 2);
        }

        void mouseEnter(const juce::MouseEvent &event) override
        {
            activateTooltip(true);
            z->repaint();
        }
        void mouseExit(const juce::MouseEvent &event) override
        {
            activateTooltip(false);
            z->repaint();
        }
        juce::Point<float> lmpos;
        void mouseDown(const juce::MouseEvent &event) override { lmpos = event.position; }
        void mouseDoubleClick(const juce::MouseEvent &event) override
        {
            activateTooltip(false);
            z->resetZoom();
        }
        void mouseDrag(const juce::MouseEvent &event) override
        {
            activateTooltip(false);
            auto d = event.position - lmpos;
            lmpos = event.position;

            auto cp = z->getLocalBounds().getCentre().toFloat();
            static constexpr float magFac{0.005f};
            if (fabs(d.x) > fabs(d.y))
            {
                z->adjustHorizontalZoom(cp, 1 + magFac * d.x);
            }
            else
            {
                z->adjustVerticalZoom(cp, 1 - magFac * d.y);
            }
        }
        ZoomContainer *z;
        ScrollBar *v, *h;
        std::unique_ptr<ToolTip> toolTip;
    };

    std::unique_ptr<ScrollBar> vScroll, hScroll;
    std::unique_ptr<MagControl> magControl;
    std::unique_ptr<ZoomContainerClient> contents;
    ZoomContainer(std::unique_ptr<ZoomContainerClient> &&c) : contents(std::move(c))
    {
        if (contents->supportsHorizontalZoom())
        {
            hScroll = std::make_unique<ScrollBar>(false);
            hScroll->setAutoHide(false);
            hScroll->setRangeLimits(0.0, 1.0, juce::NotificationType::dontSendNotification);
            hScroll->addListener(this);
            addAndMakeVisible(*hScroll);
        }
        if (contents->supportsVerticalZoom())
        {
            vScroll = std::make_unique<ScrollBar>(true);
            vScroll->setAutoHide(false);
            vScroll->setRangeLimits(0.0, 1.0, juce::NotificationType::dontSendNotification);
            vScroll->addListener(this);
            addAndMakeVisible(*vScroll);
        }
        addAndMakeVisible(*(contents->associatedComponent()));
        if constexpr (!sst::jucegui::util::onMac)
        {
            // for the middle-button pan; the contents covers us so it sees the events first
            contents->associatedComponent()->addMouseListener(this, true);
        }

        if (hScroll || vScroll)
        {
            magControl = std::make_unique<MagControl>(this, vScroll.get(), hScroll.get());
            addAndMakeVisible(*magControl);
        }
    }

    static constexpr int scrollBarWidth{6}, scrollBarMargin{2};

    void resized() override
    {
        auto bx = getLocalBounds();

        auto trim = scrollBarWidth + scrollBarMargin;
        if (hScroll && vScroll)
        {
            auto hb = bx.withTrimmedTop(bx.getHeight() - scrollBarWidth).withTrimmedRight(trim);
            auto vb = bx.withTrimmedLeft(bx.getWidth() - scrollBarWidth).withTrimmedBottom(trim);
            auto mc = bx.withTrimmedTop(bx.getHeight() - scrollBarWidth - 3)
                          .withTrimmedLeft(bx.getWidth() - scrollBarWidth - 3);
            bx = bx.withTrimmedBottom(trim).withTrimmedRight(trim);
            hScroll->setBounds(hb);
            vScroll->setBounds(vb);
            magControl->setBounds(mc);
        }
        else if (hScroll)
        {
            auto hb = bx.withTrimmedTop(bx.getHeight() - scrollBarWidth);
            auto mc = bx.withTrimmedTop(bx.getHeight() - scrollBarWidth - 3)
                          .withTrimmedLeft(bx.getWidth() - scrollBarWidth - 3);
            bx = bx.withTrimmedBottom(trim);
            hScroll->setBounds(hb);
            magControl->setBounds(mc);
        }
        else if (vScroll)
        {
            auto vb = bx.withTrimmedLeft(bx.getWidth() - scrollBarWidth);
            auto mc = bx.withTrimmedTop(bx.getHeight() - scrollBarWidth - 3)
                          .withTrimmedLeft(bx.getWidth() - scrollBarWidth - 3);
            bx = bx.withTrimmedRight(trim);
            vScroll->setBounds(vb);
            magControl->setBounds(mc);
        }
        contents->associatedComponent()->setBounds(bx);
    }

    void mouseMagnify(const juce::MouseEvent &event, float scaleFactor) override
    {
        if (event.mods.isShiftDown())
        {
            adjustVerticalZoom(event.position, scaleFactor);
        }
        else
        {
            adjustHorizontalZoom(event.position, scaleFactor);
        }
        Component::mouseMagnify(event, scaleFactor);
    }

    // Move a scrollbar by a fraction of its currently visible range, so the
    // apparent speed stays the same however far you are zoomed in.
    void scrollByVisibleFraction(ScrollBar *sb, double fraction)
    {
        if (!sb)
            return;
        auto rs = sb->getCurrentRangeStart();
        auto rw = sb->getCurrentRangeSize();
        sb->setCurrentRangeStart(std::clamp(rs + fraction * rw, 0., 1.));
    }

    void mouseWheelMove(const juce::MouseEvent &event,
                        const juce::MouseWheelDetails &wheel) override
    {
        /* Evil Dragon spec for windows:
         *
         * Mousewheel - scroll up-down (even though one could argue in Sample view it would be far
         * more useful if it did zoom in/out, straight up)
         * Shift+Mousewheel - scroll left-right
         * Alt+Mousewheel - vertical zoom in-out
         * Shift+Alt+Mousewheel - horizontal zoom in-out
         *
         * HMousewheel - scroll left-right (this is backwards with mac code)
         * Alt+HMousewheel - zoom horizontally
         */

        if constexpr (sst::jucegui::util::onMac)
        {
            static constexpr float wheelFac{1.f}; // adjust zoom by this much on win

            if (fabs(wheel.deltaX) < fabs(wheel.deltaY))
            {
                // OK so we have a vertical-style motion
                bool doHZoom{false}, doVZoom{false}, doHScroll{false};
                // on macOS shift-wheel gives you deltay so just handle the alt zoom case
                auto ad = event.mods.isAltDown();
                doVZoom = ad;

                if (doHZoom)
                {
                    // HZOOM
                    if (contents->supportsHorizontalZoom())
                    {
                        adjustHorizontalZoom(event.position, 1.0 + wheelFac * wheel.deltaY);
                    }
                }
                else if (doVZoom)
                {
                    // VZoom by delta Y
                    if (contents->supportsVerticalZoom())
                    {
                        adjustVerticalZoom(event.position, 1.0 + wheelFac * wheel.deltaY);
                    }
                }
                else if (doHScroll)
                {
                    // HSCROLL
                    if (hScroll)
                    {
                        auto dy = -wheel.deltaY;
                        auto rs = hScroll->getCurrentRangeStart();
                        auto rw = hScroll->getCurrentRangeSize();

                        // You want translation to be relative to the size to make
                        // it sort of "uniform speed"
                        rs = std::clamp(rs - wheelFac * dy * rw * 2, 0., 1.);
                        hScroll->setCurrentRangeStart(rs);
                    }
                }
                else
                {
                    // VSCROLL
                    if (vScroll)
                    {
                        auto dy = wheel.deltaY;
                        auto rs = vScroll->getCurrentRangeStart();
                        auto rw = vScroll->getCurrentRangeSize();

                        rs = std::clamp(rs - wheelFac * dy * rw * 2, 0., 1.);
                        vScroll->setCurrentRangeStart(rs);
                    }
                }
            }
            else
            {
                // on macOS shift-wheel gives you deltay so we assume alt-hdrag
                // maps also to shift-alt-vdrag which is an hzoom gesture
                auto ad = event.mods.isAltDown();

                if (ad)
                {
                    // HZOOM
                    if (contents->supportsHorizontalZoom())
                    {
                        adjustHorizontalZoom(event.position, 1.0 + wheelFac * wheel.deltaX);
                    }
                }
                else
                {
                    if (hScroll)
                    {
                        auto dy = wheel.deltaX;
                        auto rs = hScroll->getCurrentRangeStart();
                        auto rw = hScroll->getCurrentRangeSize();

                        // You want translation to be relative to the size to make
                        // it sort of "uniform speed"
                        rs = std::clamp(rs - dy * rw * 2, 0., 1.);
                        hScroll->setCurrentRangeStart(rs);
                    }
                }
            }
        }
        else
        {
            namespace jutil = sst::jucegui::util;

            // per detent of a notched wheel
            static constexpr double zoomPerDetent{0.1};
            static constexpr double scrollPerDetent{0.15};

            const auto dyDet = jutil::wheelDetents(event, wheel);
            const auto dxDet = jutil::wheelDetentsX(event, wheel);

            const auto ad = event.mods.isAltDown();
            const auto sd = event.mods.isShiftDown();

            if (fabs(wheel.deltaX) < fabs(wheel.deltaY))
            {
                const bool doHZoom = sd && ad;
                const bool doVZoom = ad && !doHZoom;
                const bool doHScroll = sd && !doHZoom && !doVZoom;

                if (doHZoom)
                {
                    if (contents->supportsHorizontalZoom())
                        adjustHorizontalZoom(event.position, 1.0 + zoomPerDetent * dyDet);
                }
                else if (doVZoom)
                {
                    if (contents->supportsVerticalZoom())
                        adjustVerticalZoom(event.position, 1.0 + zoomPerDetent * dyDet);
                }
                else if (doHScroll)
                {
                    /*
                     * juce::Viewport maps a shift-wheel to "pos.x -= deltaY", so wheel
                     * up scrolls left. We used to do the opposite of that, and so of
                     * every other app on these platforms.
                     */
                    scrollByVisibleFraction(hScroll.get(), -scrollPerDetent * dyDet);
                }
                else
                {
                    scrollByVisibleFraction(vScroll.get(), -scrollPerDetent * dyDet);
                }
            }
            else
            {
                if (sd)
                {
                    if (contents->supportsHorizontalZoom())
                        adjustHorizontalZoom(event.position, 1.0 + zoomPerDetent * dxDet);
                }
                else
                {
                    // and "pos.x -= deltaX" for a tilt wheel or a sideways swipe
                    scrollByVisibleFraction(hScroll.get(), -scrollPerDetent * dxDet);
                }
            }
        }
    }

    /*
     * Middle button drag pans the zoomed contents, the way it does in most
     * image and timeline editors. We listen on the contents rather than
     * handling it here because the contents fills us entirely and so takes
     * every mouse event.
     */
    void mouseDown(const juce::MouseEvent &e) override
    {
        if constexpr (sst::jucegui::util::onMac)
            return;

        if (!e.mods.isMiddleButtonDown() || middlePanActive)
            return;

        middlePanActive = true;
        middlePanLast = e.getEventRelativeTo(this).position;

        /*
         * Set the cursor on whatever is actually under the pointer rather than
         * on the contents. The contents usually has a child covering it, and
         * it is the innermost component that decides the cursor.
         */
        middlePanCursorTarget = e.originalComponent;
        if (middlePanCursorTarget)
        {
            middlePanPriorCursor = middlePanCursorTarget->getMouseCursor();
            middlePanCursorTarget->setMouseCursor(juce::MouseCursor::DraggingHandCursor);
        }
    }

    void mouseDrag(const juce::MouseEvent &e) override
    {
        if (!middlePanActive)
            return;

        auto p = e.getEventRelativeTo(this).position;
        auto d = p - middlePanLast;
        middlePanLast = p;

        // grabbing the contents and pulling right brings in what is off to the left
        if (hScroll)
        {
            auto w = std::max(1, getWidth() - (vScroll ? scrollBarWidth : 0));
            scrollByVisibleFraction(hScroll.get(), -d.x / w);
        }
        if (vScroll)
        {
            auto h = std::max(1, getHeight() - (hScroll ? scrollBarWidth : 0));
            scrollByVisibleFraction(vScroll.get(), -d.y / h);
        }
    }

    void mouseUp(const juce::MouseEvent &e) override
    {
        if (!middlePanActive)
            return;

        middlePanActive = false;
        if (middlePanCursorTarget)
            middlePanCursorTarget->setMouseCursor(middlePanPriorCursor);
        middlePanCursorTarget = nullptr;
    }

    bool middlePanActive{false};
    juce::Point<float> middlePanLast;
    juce::MouseCursor middlePanPriorCursor;
    juce::Component::SafePointer<juce::Component> middlePanCursorTarget;

    void scrollBarMoved(juce::ScrollBar *scrollBarThatHasMoved, double newRangeStart) override
    {
        if (scrollBarThatHasMoved == hScroll.get())
        {
            contents->setHorizontalZoom(newRangeStart, 1.0 / hScroll->getCurrentRangeSize());
        }
        if (scrollBarThatHasMoved == vScroll.get())
        {
            contents->setVerticalZoom(newRangeStart, 1.0 / vScroll->getCurrentRangeSize());
        }
    }

    double hZoomFloor{0.0}, vZoomFloor{0};
    /**
     * Set the smallest zoom as a percentage of the total window. So 0.1
     * means you cap out at 10x zoom. And (24+2)/128 means you cap out at
     * 2 octaves and 2 notes if your span is 128 notes.
     *
     * @param f The zoom
     */
    void setHZoomFloor(float f) { hZoomFloor = std::clamp(f, 0.f, 1.f); }
    void setVZoomFloor(float f) { vZoomFloor = std::clamp(f, 0.f, 1.f); }

    void adjustVerticalZoom(const juce::Point<float> &p, float scaleFactor)
    {
        if (!vScroll)
            return;

        auto rs = vScroll->getCurrentRangeStart();
        auto ors = rs;
        auto re = vScroll->getCurrentRangeSize();

        auto nre = re / scaleFactor;
        auto dre = re - nre;
        re = nre;

        auto mfac = std::clamp(p.getY() / (getHeight() - (hScroll ? scrollBarWidth : 0)), 0.f, 1.f);

        rs += dre * mfac;

        rs = std::clamp(rs, 0., 1.);
        re = std::clamp(re, vZoomFloor, 1.);
        if (re <= vZoomFloor)
            rs = ors;

        contents->setVerticalZoom(rs, 1.0 / re);

        vScroll->setCurrentRange(rs, re, juce::NotificationType::dontSendNotification);
        vScroll->repaint();
    }
    void adjustHorizontalZoom(const juce::Point<float> &p, float scaleFactor)
    {
        if (!hScroll)
            return;

        auto rs = hScroll->getCurrentRangeStart();
        auto ors = rs;
        auto re = hScroll->getCurrentRangeSize();

        auto nre = re / scaleFactor;
        auto dre = re - nre;
        re = nre;

        auto mfac = std::clamp(p.getX() / (getWidth() - (vScroll ? scrollBarWidth : 0)), 0.f, 1.f);

        rs += dre * mfac;

        rs = std::clamp(rs, 0., 1.);
        re = std::clamp(re, hZoomFloor, 1.);
        if (re <= hZoomFloor)
            rs = ors;

        contents->setHorizontalZoom(rs, 1.0 / re);

        hScroll->setCurrentRange(rs, re, juce::NotificationType::dontSendNotification);
        hScroll->repaint();
    }

    void resetZoom()
    {
        if (hScroll)
        {
            contents->setHorizontalZoom(0.0, 1.0);
            hScroll->setCurrentRange(0.0, 1.0, juce::NotificationType::dontSendNotification);
            hScroll->repaint();
        }
        if (vScroll)
        {
            contents->setVerticalZoom(0.0, 1.0);
            vScroll->setCurrentRange(0.0, 1.0, juce::NotificationType::dontSendNotification);
            vScroll->repaint();
        }
        repaint();
    }
};
} // namespace sst::jucegui::components
#endif // SST_JUCEGUI_ZOOMCONTAINER_H
