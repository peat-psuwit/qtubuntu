/*
 * Copyright (C) 2015 Canonical, Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3, as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranties of MERCHANTABILITY,
 * SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "cursor.h"

#include "logging.h"
#include "window.h"

#include <mir_toolkit/mir_client_library.h>

UbuntuCursor::UbuntuCursor(MirConnection *connection)
    : mConnection(connection)
{
    mShapeToCursorName[Qt::ArrowCursor] = "left_ptr";
    mShapeToCursorName[Qt::UpArrowCursor] = "up_arrow";
    mShapeToCursorName[Qt::CrossCursor] = "cross";
    mShapeToCursorName[Qt::WaitCursor] = "watch";
    mShapeToCursorName[Qt::IBeamCursor] = "xterm";
    mShapeToCursorName[Qt::SizeVerCursor] = "size_ver";
    mShapeToCursorName[Qt::SizeHorCursor] = "size_hor";
    mShapeToCursorName[Qt::SizeBDiagCursor] = "size_bdiag";
    mShapeToCursorName[Qt::SizeFDiagCursor] = "size_fdiag";
    mShapeToCursorName[Qt::SizeAllCursor] = "size_all";
    mShapeToCursorName[Qt::BlankCursor] = "blank";
    mShapeToCursorName[Qt::SplitVCursor] = "split_v";
    mShapeToCursorName[Qt::SplitHCursor] = "split_h";
    mShapeToCursorName[Qt::PointingHandCursor] = "hand";
    mShapeToCursorName[Qt::ForbiddenCursor] = "forbidden";
    mShapeToCursorName[Qt::WhatsThisCursor] = "whats_this";
    mShapeToCursorName[Qt::BusyCursor] = "left_ptr_watch";
    mShapeToCursorName[Qt::OpenHandCursor] = "openhand";
    mShapeToCursorName[Qt::ClosedHandCursor] = "closedhand";
    mShapeToCursorName[Qt::DragCopyCursor] = "dnd-copy";
    mShapeToCursorName[Qt::DragMoveCursor] = "dnd-move";
    mShapeToCursorName[Qt::DragLinkCursor] = "dnd-link";
}

namespace {
#if !defined(QT_NO_DEBUG)
const char *qtCursorShapeToStr(Qt::CursorShape shape)
{
    switch(shape) {
    case Qt::ArrowCursor:
        return "Arrow";
    case Qt::UpArrowCursor:
        return "UpArrow";
    case Qt::CrossCursor:
        return "Cross";
    case Qt::WaitCursor:
        return "Wait";
    case Qt::IBeamCursor:
        return "IBeam";
    case Qt::SizeVerCursor:
        return "SizeVer";
    case Qt::SizeHorCursor:
        return "SizeHor";
    case Qt::SizeBDiagCursor:
        return "SizeBDiag";
    case Qt::SizeFDiagCursor:
        return "SizeFDiag";
    case Qt::SizeAllCursor:
        return "SizeAll";
    case Qt::BlankCursor:
        return "Blank";
    case Qt::SplitVCursor:
        return "SplitV";
    case Qt::SplitHCursor:
        return "SplitH";
    case Qt::PointingHandCursor:
        return "PointingHand";
    case Qt::ForbiddenCursor:
        return "Forbidden";
    case Qt::WhatsThisCursor:
        return "WhatsThis";
    case Qt::BusyCursor:
        return "Busy";
    case Qt::OpenHandCursor:
        return "OpenHand";
    case Qt::ClosedHandCursor:
        return "ClosedHand";
    case Qt::DragCopyCursor:
        return "DragCopy";
    case Qt::DragMoveCursor:
        return "DragMove";
    case Qt::DragLinkCursor:
        return "DragLink";
    case Qt::BitmapCursor:
        return "Bitmap";
    default:
        return "???";
    }
}
#endif // !defined(QT_NO_DEBUG)
} // anonymous namespace

void UbuntuCursor::changeCursor(QCursor *windowCursor, QWindow *window)
{
    if (!window) {
        return;
    }

    MirSurface *surface = static_cast<UbuntuWindow*>(window->handle())->mirSurface();

    if (!surface) {
        return;
    }


    if (windowCursor) {
        DLOG("[ubuntumirclient QPA] changeCursor shape=%s, window=%p\n", qtCursorShapeToStr(windowCursor->shape()), window);
        if (!windowCursor->pixmap().isNull()) {
            configureMirCursorWithPixmapQCursor(surface, *windowCursor);
        } else if (windowCursor->shape() == Qt::BitmapCursor) {
            // TODO: Implement bitmap cursor support
            applyDefaultCursorConfiguration(surface);
        } else {
            const auto &cursorName = mShapeToCursorName.value(windowCursor->shape(), QByteArray("left_ptr"));
            auto cursorConfiguration = mir_cursor_configuration_from_name(cursorName.data());
            mir_surface_configure_cursor(surface, cursorConfiguration);
            mir_cursor_configuration_destroy(cursorConfiguration);
        }
    } else {
        applyDefaultCursorConfiguration(surface);
    }

}

void UbuntuCursor::configureMirCursorWithPixmapQCursor(MirSurface *surface, QCursor &cursor)
{
    QImage image = cursor.pixmap().toImage();

    if (image.format() != QImage::Format_ARGB32) {
        image.convertToFormat(QImage::Format_ARGB32);
    }

    MirBufferStream *bufferStream = mir_connection_create_buffer_stream_sync(mConnection,
            image.width(), image.height(), mir_pixel_format_argb_8888, mir_buffer_usage_software);

    {
        MirGraphicsRegion region;
        mir_buffer_stream_get_graphics_region(bufferStream, &region);

        char *regionLine = region.vaddr;
        Q_ASSERT(image.bytesPerLine() <= region.stride);
        for (int i = 0; i < image.height(); ++i) {
            memcpy(regionLine, image.scanLine(i), image.bytesPerLine());
            regionLine += region.stride;
        }
    }

    mir_buffer_stream_swap_buffers_sync(bufferStream);

    {
        auto configuration = mir_cursor_configuration_from_buffer_stream(bufferStream, cursor.hotSpot().x(), cursor.hotSpot().y());
        mir_surface_configure_cursor(surface, configuration);
        mir_cursor_configuration_destroy(configuration);
    }

    mir_buffer_stream_release_sync(bufferStream);
}

void UbuntuCursor::applyDefaultCursorConfiguration(MirSurface *surface)
{
    auto cursorConfiguration = mir_cursor_configuration_from_name("left_ptr");
    mir_surface_configure_cursor(surface, cursorConfiguration);
    mir_cursor_configuration_destroy(cursorConfiguration);
}