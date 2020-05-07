/*
    Borealis, a Nintendo Switch UI Library
    Copyright (C) 2020  natinusala

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <borealis/scroll_view.hpp>
#include <borealis/application.hpp>

#include <math.h>

namespace brls
{

// TODO: update scrolling when resizing window (apply new scrollY)

void ScrollView::draw(NVGcontext* vg, int x, int y, unsigned width, unsigned height, Style* style, FrameContext* ctx)
{
    if (!this->contentView)
        return;

    // Enable scissoring
    nvgSave(vg);
    nvgScissor(vg, x, y, this->width, this->height);

    // Draw content view
    this->contentView->frame(ctx);

    //Disable scissoring
    nvgRestore(vg);
}

unsigned ScrollView::getYCenter(View* view)
{
    return view->getY() + view->getHeight() / 2;
}

void ScrollView::layout(NVGcontext* vg, Style* style, FontStash* stash)
{
    this->prebakeScrolling();

    // Layout content view
    if (this->contentView)
    {
        this->contentView->setBoundaries(
            this->getX(),
            this->getY() + roundf(this->scrollY),
            this->getWidth(),
            this->getHeight()
        );
        this->contentView->invalidate();
    }

    this->ready = true;
}

void ScrollView::willAppear()
{
    this->prebakeScrolling();

    if (this->contentView)
        this->contentView->willAppear();
}

void ScrollView::willDisappear()
{
    // Reset scrolling to the top
    this->startScrolling(false, 0.0f);

    // Send event to content view
    if (this->contentView)
        this->contentView->willDisappear();
}

View* ScrollView::getDefaultFocus()
{
    return this->contentView;
}

void ScrollView::setContentView(View* view)
{
    this->contentView = view;

    if (this->contentView)
    {
        this->contentView->setParent(this);
        this->contentView->willAppear();
    }

    this->invalidate();
}

View* ScrollView::getContentView()
{
    return this->contentView;
}

void ScrollView::prebakeScrolling()
{
    // Prebaked values for scrolling
    this->middleY = this->y + this->height / 2;
    this->bottomY = this->y + this->height;
}

void ScrollView::updateScrolling(bool animated)
{
    // Don't scroll if layout hasn't been called yet
    if (!this->ready || !this->contentView)
        return;

    View* focusedView                  = Application::getCurrentFocus();
    int currentSelectionMiddleOnScreen = focusedView->getY() + focusedView->getHeight() / 2;
    float newScroll                    = this->scrollY - ((float)currentSelectionMiddleOnScreen - (float)this->middleY);

    // Bottom boundary
    if ((float)this->y + newScroll + (float)this->contentView->getHeight() < (float)this->bottomY)
        newScroll = (float)this->height - (float)this->contentView->getHeight();

    // Top boundary
    if (newScroll > 0.0f)
        newScroll = 0.0f;

    //Start animation
    this->startScrolling(animated, newScroll);
}

void ScrollView::startScrolling(bool animated, float newScroll)
{
    if (newScroll == this->scrollY)
        return;

    menu_animation_ctx_tag tag = (uintptr_t) & this->scrollY;
    menu_animation_kill_by_tag(&tag);

    if (animated)
    {
        Style* style = Application::getStyle();

        menu_animation_ctx_entry_t entry;
        entry.cb           = [](void* userdata) {};
        entry.duration     = style->AnimationDuration.highlight;
        entry.easing_enum  = EASING_OUT_QUAD;
        entry.subject      = &this->scrollY;
        entry.tag          = tag;
        entry.target_value = newScroll;
        entry.tick         = [this](void* userdata) { this->scrollAnimationTick(); };
        entry.userdata     = nullptr;

        menu_animation_push(&entry);
    }
    else
    {
        this->scrollY = newScroll;
    }

    this->invalidate();
}

void ScrollView::scrollAnimationTick()
{
    this->invalidate();
}

void ScrollView::onChildFocusGained(View* child)
{
    // layout hasn't been called yet, don't scroll
    if (!this->ready)
        return;

    // Safety check to ensure that we don't have
    // two children (setContentView called twice)
    if (child != this->contentView)
        return;

    // Start scrolling
    this->updateScrolling(true);

    View::onChildFocusGained(child);
}

ScrollView::~ScrollView()
{
    if (this->contentView)
    {
         this->contentView->willDisappear();
         delete this->contentView;
    }
}

} // namespace brls