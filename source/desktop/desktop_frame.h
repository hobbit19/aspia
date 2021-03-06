//
// Aspia Project
// Copyright (C) 2018 Dmitry Chapyshev <dmitry@aspia.ru>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.
//

#ifndef DESKTOP__DESKTOP_FRAME_H
#define DESKTOP__DESKTOP_FRAME_H

#include "base/macros_magic.h"
#include "desktop/pixel_format.h"

#include <QRegion>

namespace desktop {

class Frame
{
public:
    virtual ~Frame() = default;

    uint8_t* frameDataAtPos(const QPoint& pos) const;
    uint8_t* frameDataAtPos(int x, int y) const;
    uint8_t* frameData() const { return data_; }
    const QSize& size() const { return size_; }
    const PixelFormat& format() const { return format_; }
    int stride() const { return stride_; }
    bool contains(int x, int y) const;

    const QRegion& constUpdatedRegion() const { return updated_region_; }
    QRegion* updatedRegion() { return &updated_region_; }

    const QPoint& topLeft() const { return top_left_; }
    void setTopLeft(const QPoint& top_left) { top_left_ = top_left; }

protected:
    Frame(const QSize& size, const PixelFormat& format, int stride, uint8_t* data);

    // Ownership of the buffers is defined by the classes that inherit from
    // this class. They must guarantee that the buffer is not deleted before
    // the frame is deleted.
    uint8_t* const data_;

private:
    const QSize size_;
    const PixelFormat format_;
    const int stride_;

    QRegion updated_region_;
    QPoint top_left_;

    DISALLOW_COPY_AND_ASSIGN(Frame);
};

} // namespace desktop

#endif // DESKTOP__DESKTOP_FRAME_H
