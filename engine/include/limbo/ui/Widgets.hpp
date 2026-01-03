#pragma once

#include "limbo/ui/Widget.hpp"
#include "limbo/core/Types.hpp"
#include "limbo/render/common/Texture.hpp"

#include <functional>

namespace limbo {

/**
 * Panel - A container widget that can hold other widgets
 */
class LIMBO_API Panel : public Widget {
public:
    Panel() {
        m_style.backgroundColor = glm::vec4(0.15f, 0.15f, 0.15f, 0.9f);
        m_style.borderColor = glm::vec4(0.3f, 0.3f, 0.3f, 1.0f);
        m_style.borderWidth = 1.0f;
    }

    void render(const glm::vec2& screenSize) override;
};

/**
 * Label - Displays text (simplified - renders as colored rectangle for now)
 *
 * Note: Full text rendering would require font loading/atlas.
 * This is a placeholder that shows the label area.
 */
class LIMBO_API Label : public Widget {
public:
    Label() {
        m_style.backgroundColor = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);  // Transparent
        m_style.borderWidth = 0.0f;
        m_interactive = false;
    }

    explicit Label(const String& text) : Label() { m_text = text; }

    void setText(const String& text) { m_text = text; }
    [[nodiscard]] const String& getText() const { return m_text; }

    void setTextColor(const glm::vec4& color) { m_style.textColor = color; }

    void render(const glm::vec2& screenSize) override;

private:
    String m_text;
};

/**
 * Button - A clickable widget with text
 */
class LIMBO_API Button : public Widget {
public:
    using ClickCallback = std::function<void()>;

    Button() {
        m_style.backgroundColor = glm::vec4(0.25f, 0.25f, 0.28f, 1.0f);
        m_style.hoverColor = glm::vec4(0.35f, 0.35f, 0.4f, 1.0f);
        m_style.pressedColor = glm::vec4(0.2f, 0.4f, 0.6f, 1.0f);
        m_style.borderColor = glm::vec4(0.4f, 0.4f, 0.45f, 1.0f);
        m_style.borderWidth = 1.0f;
    }

    explicit Button(const String& text) : Button() { m_text = text; }

    void setText(const String& text) { m_text = text; }
    [[nodiscard]] const String& getText() const { return m_text; }

    void setOnClick(ClickCallback callback) { m_onClick = std::move(callback); }

    bool onMouseUp(const glm::vec2& mousePos, const glm::vec2& screenSize) override;
    void render(const glm::vec2& screenSize) override;

private:
    String m_text;
    ClickCallback m_onClick;
};

/**
 * ProgressBar - Shows a progress value between 0 and 1
 */
class LIMBO_API ProgressBar : public Widget {
public:
    ProgressBar() {
        m_style.backgroundColor = glm::vec4(0.1f, 0.1f, 0.1f, 0.9f);
        m_style.borderColor = glm::vec4(0.3f, 0.3f, 0.3f, 1.0f);
        m_style.borderWidth = 1.0f;
        m_interactive = false;
        m_size = glm::vec2(200.0f, 20.0f);
    }

    void setProgress(f32 progress) { m_progress = glm::clamp(progress, 0.0f, 1.0f); }
    [[nodiscard]] f32 getProgress() const { return m_progress; }

    void setFillColor(const glm::vec4& color) { m_fillColor = color; }
    [[nodiscard]] const glm::vec4& getFillColor() const { return m_fillColor; }

    void render(const glm::vec2& screenSize) override;

private:
    f32 m_progress = 0.5f;
    glm::vec4 m_fillColor{0.2f, 0.6f, 0.9f, 1.0f};
};

/**
 * Image - Displays a texture
 */
class LIMBO_API Image : public Widget {
public:
    Image() {
        m_style.backgroundColor = glm::vec4(1.0f);
        m_style.borderWidth = 0.0f;
        m_interactive = false;
    }

    void setTexture(Texture2D* texture) { m_texture = texture; }
    [[nodiscard]] Texture2D* getTexture() const { return m_texture; }

    void setTint(const glm::vec4& tint) { m_style.backgroundColor = tint; }

    void render(const glm::vec2& screenSize) override;

private:
    Texture2D* m_texture = nullptr;
};

}  // namespace limbo
