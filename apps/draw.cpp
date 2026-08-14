/**
 *  Copyright 2015 Mike Reed
 */

#include "GWindow.h"
#include "../include/GBase64.h"
#include "../include/GBitmap.h"
#include "../include/GCanvas.h"
#include "../include/GColor.h"
#include "../include/GRandom.h"
#include "../include/GRect.h"

#include <functional>
#include <vector>

using GRedrawProc = std::function<void()>;

static const float CORNER_SIZE = 9;

template <typename T> int find_index(const std::vector<T*>& list, T* target) {
    for (int i = 0; i < list.size(); ++i) {
        if (list[i] == target) {
            return i;
        }
    }
    return -1;
}

static GRandom gRand;

static GColor rand_color() {
    return {gRand.nextF(), gRand.nextF(), gRand.nextF(), 0.5f};
}

static GRect make_from_pts(const GPoint& p0, const GPoint& p1) {
    return GRect::LTRB(std::min(p0.x, p1.x), std::min(p0.y, p1.y),
                       std::max(p0.x, p1.x), std::max(p0.y, p1.y));
}

static bool contains(const GRect& rect, GPoint p) {
    return rect.left < p.x && p.x < rect.right && rect.top < p.y && p.y < rect.bottom;
}

static bool hit_test(GPoint a, GPoint b) {
    return (b - a).length() <= CORNER_SIZE;
}

static bool in_resize_corner(const GRect& r, GPoint p, GPoint* anchor) {
    if (hit_test(r.TL(), p)) {
        *anchor = r.BR();
        return true;
    } else if (hit_test(r.TR(), p)) {
        *anchor = r.BL();
        return true;
    } else if (hit_test(r.BR(), p)) {
        *anchor = r.TL();
        return true;
    } else if (hit_test(r.BL(), p)) {
        *anchor = r.TR();
        return true;
    }
    return false;
}

static void draw_corner(GCanvas* canvas, const GColor& c, GPoint p, float dx, float dy) {
    canvas->fillRect(make_from_pts({p.x, p.y - 1}, {p.x + dx, p.y + 1}), c);
    canvas->fillRect(make_from_pts({p.x - 1, p.y}, {p.x + 1, p.y + dy}), c);
}

static void draw_hilite(GCanvas* canvas, const GRect& r) {
    const float size = CORNER_SIZE;
    GColor c = {0, 0, 0, 1};
    draw_corner(canvas, c, r.TL(), size, size);
    draw_corner(canvas, c, r.BL(), size, -size);
    draw_corner(canvas, c, r.TR(), -size, size);
    draw_corner(canvas, c, r.BR(), -size, -size);
}

static void constrain_color(GColor* c) {
    c->a = std::max(std::min(c->a, 1.f), 0.1f);
    c->r = std::max(std::min(c->r, 1.f), 0.f);
    c->g = std::max(std::min(c->g, 1.f), 0.f);
    c->b = std::max(std::min(c->b, 1.f), 0.f);
}

class Shape {
public:
    virtual ~Shape() {}
    virtual void draw(GCanvas* canvas) {}
    virtual void drawHilite(GCanvas*) {}
    virtual bool hitTest(GPoint) { return false; }
    virtual void offset(GVector) {}
    virtual GColor getColor() = 0;
    virtual void setColor(const GColor&) {}

    virtual GClick* findClick(GPoint, GRedrawProc) { return nullptr; }
};

class RectShape : public Shape {
public:
    RectShape(GPoint p, GColor c) : fColor(c) {
        fRect = {p.x, p.y, p.x, p.y};
    }

    void draw(GCanvas* canvas) override {
        canvas->fillRect(fRect, fColor);
    }

    void drawHilite(GCanvas* canvas) override {
        draw_hilite(canvas, fRect);
    }

    bool hitTest(GPoint p) override { return contains(fRect, p); }
    void offset(GVector v) override {
        fRect.left += v.x;
        fRect.top += v.y;
        fRect.right += v.x;
        fRect.bottom += v.y;
    }

    GColor getColor() override { return fColor; }
    void setColor(const GColor& c) override { fColor = c; }

    GClick* findClick(GPoint loc, GRedrawProc proc) override {
        GPoint anchor;
        if (in_resize_corner(fRect, loc, &anchor)) {
            return new GClick(loc, [this, anchor, proc](GClick* click) {
                fRect = make_from_pts(click->curr(), anchor);
                proc();
            });
        }
        return nullptr;
    }
private:
    GRect   fRect;
    GColor  fColor;
};

static inline float dot(GVector a, GVector b) { return a.x * b.x + a.y * b.y; }
static inline float cross(GVector a, GVector b) { return a.x * b.y - a.y * b.x; }

class LineShape : public Shape {
public:
    LineShape(GPoint p, GColor c) : fColor(c) {
        c.a = 1;    // make it easier to see for now
        fPts[0] = fPts[1] = p;
    }

    void draw(GCanvas* canvas) override {
        canvas->hairLine(fPts[0], fPts[1], fColor);
    }

    void drawHilite(GCanvas* canvas) override {
        const float r = 3;
        GColor c = {0, 0, 0, 1};
        auto draw_point = [&](GPoint p) {
            canvas->fillRect({p.x - r, p.y - r, p.x + r, p.y + r}, c);
        };
        for (auto p : fPts) {
            draw_point(p);
        }
    }

    GColor getColor() override { return fColor; }
    void setColor(const GColor& c) override { fColor = c; }

    bool hitTest(GPoint p) override {
        const float tol = 5;
        auto base = fPts[1] - fPts[0];
        auto hypt = p - fPts[0];
        auto len = base.length();
        auto dist = std::abs(cross(p - fPts[0], base)/len);
        auto proj = dot(base, hypt) / len;
        return dist <= tol && proj >= 0 && proj <= len;
    }

    void offset(GVector v) override {
        for (auto& p : fPts) {
            p += v;
        }
    }

    GClick* findClick(GPoint loc, GRedrawProc proc) override {
        for (size_t i = 0; i < fPts.size(); ++i) {
            if (hit_test(fPts[i], loc)) {
                return new GClick(loc, [this, i, proc](GClick* click) {
                    fPts[i] = click->curr();
                    proc();
                });
            }
        }
        return nullptr;
    }

private:
    std::array<GPoint, 2> fPts;
    GColor                fColor;
};

static void test_base64(const void* data, size_t size) {
    auto base = GEncodeToBase64(data, size);
    auto data2 = GDecodeFromBase64(base->data(), base->size());
    assert(size == data2->size());
    assert(memcmp(data, data2->data(), size) == 0);
}

static void tests() {
    const char data[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    for (size_t i = 1; i < std::size(data); ++i) {
        test_base64(data, i);
    }
}

class TestWindow : public GWindow {
    std::vector<Shape*> fList;
    Shape* fShape;
    GColor fBGColor;

    void copyToClipboard() {
        tests();
        GBitmap bm(this->size());

        auto saveShape = fShape;
        fShape = nullptr;
        this->onUpdate(GCreateCanvas(bm).get());
        fShape = saveShape;

        auto data = bm.asPNGData();
        printf("png %zu\n", data->size());

        test_base64(data->data(), data->size());

        auto base = GEncodeToBase64(data->data(), data->size(), "data:image/png;base64,");
        printf("%s\n", (const char*)base->data());

        SDL_SetClipboardText((const char*)base->data());
    }
public:
    TestWindow(GISize sz) : GWindow(sz) {
        fBGColor = {1, 1, 1, 1};
        fShape = NULL;
    }

    virtual ~TestWindow() {}
    
protected:
    void onDraw(GCanvas* canvas) override {
        canvas->fillRect(GRect::XYWH(0, 0, 10000, 10000), fBGColor);

        for (auto shape : fList) {
            shape->draw(canvas);
        }
        if (fShape) {
            fShape->drawHilite(canvas);
        }
    }

    bool onKeyPress(uint32_t sym) override {
        if (fShape) {
            switch (sym) {
                case SDLK_UP: {
                    int index = find_index(fList, fShape);
                    if (index < fList.size() - 1) {
                        std::swap(fList[index], fList[index + 1]);
                        this->requestDraw();
                        return true;
                    }
                    return false;
                }
                case SDLK_DOWN: {
                    int index = find_index(fList, fShape);
                    if (index > 0) {
                        std::swap(fList[index], fList[index - 1]);
                        this->requestDraw();
                        return true;
                    }
                    return false;
                }
                case SDLK_DELETE:
                case SDLK_BACKSPACE:
                    this->removeShape(fShape);
                    fShape = NULL;
                    this->updateTitle();
                    this->requestDraw();
                    return true;
                default:
                    break;
            }
        }

        GColor c = fShape ? fShape->getColor() : fBGColor;
        const float delta = 0.1f;
        switch (sym) {
            case 'a': c.a -= delta; break;
            case 'A': c.a += delta; break;
            case 'r': c.r -= delta; break;
            case 'R': c.r += delta; break;
            case 'g': c.g -= delta; break;
            case 'G': c.g += delta; break;
            case 'b': c.b -= delta; break;
            case 'B': c.b += delta; break;
            case 'c': copyToClipboard(); break;
            default:
                return false;
        }
        constrain_color(&c);
        if (fShape) {
            fShape->setColor(c);
        } else {
            c.a = 1;   // need the bg to stay opaque
            fBGColor = c;
        }
        this->updateTitle();
        this->requestDraw();
        return true;
    }

    GClick* onFindClickHandler(GPoint loc) override {
        auto redraw = [this]() {
            this->updateTitle();
            this->requestDraw();
        };

        if (fShape) {
            if (auto c = fShape->findClick(loc, redraw)) {
                return c;
            }
        }

        for (int i = fList.size() - 1; i >= 0; --i) {
            if (fList[i]->hitTest(loc)) {
                fShape = fList[i];
                this->updateTitle();
                return new GClick(loc, [this](GClick* click) {
                    fShape->offset(click->curr() - click->prev());
                    this->updateTitle();
                    this->requestDraw();
                });
            }
        }
        
        // else create a new shape
        if (this->pollFastKeys() & GFastKeys::ctrl_key) {
            fShape = new LineShape(loc, rand_color());
        } else {
            fShape = new RectShape(loc, rand_color());
        }
        fList.push_back(fShape);
        return fShape->findClick(loc, redraw);
    }

private:
    void removeShape(Shape* target) {
        assert(target);

        std::vector<Shape*>::iterator it = std::find(fList.begin(), fList.end(), target);
        if (it != fList.end()) {
            fList.erase(it);
        } else {
            assert(!"shape not found?");
        }
    }

    void updateTitle() {
        char buffer[100];
        buffer[0] = ' ';
        buffer[1] = 0;

        GColor c = fBGColor;
        if (fShape) {
            c = fShape->getColor();
        }

        snprintf(buffer, sizeof(buffer), "R:%02X  G:%02X  B:%02X  A:%02X",
                int(c.r * 255), int(c.g * 255), int(c.b * 255), int(c.a * 255));
        this->setTitle(buffer);
    }

    typedef GWindow INHERITED;
};

int main(int argc, char const* const* argv) {
    GWindow* wind = new TestWindow({640, 480});

    return wind->run();
}
