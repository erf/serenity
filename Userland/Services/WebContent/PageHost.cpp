/*
 * Copyright (c) 2020-2021, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "PageHost.h"
#include "ClientConnection.h"
#include <LibGfx/Painter.h>
#include <LibGfx/SystemTheme.h>
#include <LibWeb/Cookie/ParsedCookie.h>
#include <LibWeb/Layout/InitialContainingBlockBox.h>
#include <LibWeb/Page/Frame.h>
#include <WebContent/WebContentClientEndpoint.h>

namespace WebContent {

PageHost::PageHost(ClientConnection& client)
    : m_client(client)
    , m_page(make<Web::Page>(*this))
{
    setup_palette();
}

PageHost::~PageHost()
{
}

void PageHost::setup_palette()
{
    // FIXME: Get the proper palette from our peer somehow
    auto buffer = Core::AnonymousBuffer::create_with_size(sizeof(Gfx::SystemTheme));
    auto* theme = buffer.data<Gfx::SystemTheme>();
    theme->color[(int)Gfx::ColorRole::Window] = Color::Magenta;
    theme->color[(int)Gfx::ColorRole::WindowText] = Color::Cyan;
    m_palette_impl = Gfx::PaletteImpl::create_with_anonymous_buffer(buffer);
}

Gfx::Palette PageHost::palette() const
{
    return Gfx::Palette(*m_palette_impl);
}

void PageHost::set_palette_impl(const Gfx::PaletteImpl& impl)
{
    m_palette_impl = impl;
}

Web::Layout::InitialContainingBlockBox* PageHost::layout_root()
{
    auto* document = page().main_frame().document();
    if (!document)
        return nullptr;
    return document->layout_node();
}

void PageHost::paint(const Gfx::IntRect& content_rect, Gfx::Bitmap& target)
{
    Gfx::Painter painter(target);
    Gfx::IntRect bitmap_rect { {}, content_rect.size() };

    auto* layout_root = this->layout_root();
    if (!layout_root) {
        painter.fill_rect(bitmap_rect, Color::White);
        return;
    }

    Web::PaintContext context(painter, palette(), content_rect.top_left());
    context.set_should_show_line_box_borders(m_should_show_line_box_borders);
    context.set_viewport_rect(content_rect);
    layout_root->paint_all_phases(context);
}

void PageHost::set_viewport_rect(const Gfx::IntRect& rect)
{
    page().main_frame().set_viewport_rect(rect);
}

void PageHost::page_did_invalidate(const Gfx::IntRect& content_rect)
{
    m_client.post_message(Messages::WebContentClient::DidInvalidateContentRect(content_rect));
}

void PageHost::page_did_change_selection()
{
    m_client.post_message(Messages::WebContentClient::DidChangeSelection());
}

void PageHost::page_did_request_cursor_change(Gfx::StandardCursor cursor)
{
    m_client.post_message(Messages::WebContentClient::DidRequestCursorChange((u32)cursor));
}

void PageHost::page_did_layout()
{
    auto* layout_root = this->layout_root();
    VERIFY(layout_root);
    auto content_size = enclosing_int_rect(layout_root->absolute_rect()).size();
    m_client.post_message(Messages::WebContentClient::DidLayout(content_size));
}

void PageHost::page_did_change_title(const String& title)
{
    m_client.post_message(Messages::WebContentClient::DidChangeTitle(title));
}

void PageHost::page_did_request_scroll(int wheel_delta)
{
    m_client.post_message(Messages::WebContentClient::DidRequestScroll(wheel_delta));
}

void PageHost::page_did_request_scroll_into_view(const Gfx::IntRect& rect)
{
    m_client.post_message(Messages::WebContentClient::DidRequestScrollIntoView(rect));
}

void PageHost::page_did_enter_tooltip_area(const Gfx::IntPoint& content_position, const String& title)
{
    m_client.post_message(Messages::WebContentClient::DidEnterTooltipArea(content_position, title));
}

void PageHost::page_did_leave_tooltip_area()
{
    m_client.post_message(Messages::WebContentClient::DidLeaveTooltipArea());
}

void PageHost::page_did_hover_link(const URL& url)
{
    m_client.post_message(Messages::WebContentClient::DidHoverLink(url));
}

void PageHost::page_did_unhover_link()
{
    m_client.post_message(Messages::WebContentClient::DidUnhoverLink());
}

void PageHost::page_did_click_link(const URL& url, const String& target, unsigned modifiers)
{
    m_client.post_message(Messages::WebContentClient::DidClickLink(url, target, modifiers));
}

void PageHost::page_did_middle_click_link(const URL& url, [[maybe_unused]] const String& target, [[maybe_unused]] unsigned modifiers)
{
    m_client.post_message(Messages::WebContentClient::DidMiddleClickLink(url, target, modifiers));
}

void PageHost::page_did_start_loading(const URL& url)
{
    m_client.post_message(Messages::WebContentClient::DidStartLoading(url));
}

void PageHost::page_did_finish_loading(const URL& url)
{
    m_client.post_message(Messages::WebContentClient::DidFinishLoading(url));
}

void PageHost::page_did_request_context_menu(const Gfx::IntPoint& content_position)
{
    m_client.post_message(Messages::WebContentClient::DidRequestContextMenu(content_position));
}

void PageHost::page_did_request_link_context_menu(const Gfx::IntPoint& content_position, const URL& url, const String& target, unsigned modifiers)
{
    m_client.post_message(Messages::WebContentClient::DidRequestLinkContextMenu(content_position, url, target, modifiers));
}

void PageHost::page_did_request_alert(const String& message)
{
    m_client.send_sync<Messages::WebContentClient::DidRequestAlert>(message);
}

bool PageHost::page_did_request_confirm(const String& message)
{
    return m_client.send_sync<Messages::WebContentClient::DidRequestConfirm>(message)->result();
}

String PageHost::page_did_request_prompt(const String& message, const String& default_)
{
    return m_client.send_sync<Messages::WebContentClient::DidRequestPrompt>(message, default_)->response();
}

void PageHost::page_did_change_favicon(const Gfx::Bitmap& favicon)
{
    m_client.post_message(Messages::WebContentClient::DidChangeFavicon(favicon.to_shareable_bitmap()));
}

void PageHost::page_did_request_image_context_menu(const Gfx::IntPoint& content_position, const URL& url, const String& target, unsigned modifiers, const Gfx::Bitmap* bitmap)
{
    m_client.post_message(Messages::WebContentClient::DidRequestImageContextMenu(content_position, url, target, modifiers, bitmap->to_shareable_bitmap()));
}

String PageHost::page_did_request_cookie(const URL& url, Web::Cookie::Source source)
{
    return m_client.send_sync<Messages::WebContentClient::DidRequestCookie>(url, static_cast<u8>(source))->cookie();
}

void PageHost::page_did_set_cookie(const URL& url, const Web::Cookie::ParsedCookie& cookie, Web::Cookie::Source source)
{
    m_client.post_message(Messages::WebContentClient::DidSetCookie(url, cookie, static_cast<u8>(source)));
}

}
