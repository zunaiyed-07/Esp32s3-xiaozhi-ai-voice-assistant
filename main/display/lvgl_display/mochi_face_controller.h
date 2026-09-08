#ifndef MOCHI_FACE_CONTROLLER_H
#define MOCHI_FACE_CONTROLLER_H

#include <lvgl.h>

#include <array>
#include <cstdint>

class MochiFaceController {
public:
    enum class Expression {
        NORMAL, HAPPY, SAD, ANGRY, CRYING, SILLY, SURPRISED, SLEEPY,
        LOVE, CONFUSED, LAUGHING, THINKING, LISTENING, SPEAKING
    };

    explicit MochiFaceController(lv_obj_t* parent = nullptr);
    ~MochiFaceController();

    void SetExpression(Expression expression, uint32_t transition_ms = 240);
    void SetExpression(const char* expression, uint32_t transition_ms = 240);
        void SetSpeakingMouthAnimation(bool enabled) { speaking_mouth_animation_ = enabled; }
        void SetAnimationSpeed(int speed_percent);
    Expression expression() const { return target_expression_; }
    void Update(uint32_t elapsed_ms = 16);

    void RegisterChromeObject(lv_obj_t* object);
    void EnterFullscreenFace(uint32_t duration_ms = 220);
    void ExitFullscreenFace(uint32_t duration_ms = 220);
    lv_obj_t* root() const { return root_; }

private:
    static constexpr size_t kChromeObjectCount = 16;

    void CreateFaceObjects();
    void Render();
    void AnimateOpacity(lv_obj_t* object, uint8_t from, uint8_t to, uint32_t duration_ms, bool hide_at_end);
    static void HideAnimationComplete(lv_anim_t* animation);
    static void TimerCallback(lv_timer_t* timer);
    static Expression ParseExpression(const char* expression);

    lv_obj_t* root_ = nullptr;
    lv_obj_t* left_eye_ = nullptr;
    lv_obj_t* right_eye_ = nullptr;
    lv_obj_t* left_pupil_ = nullptr;
    lv_obj_t* right_pupil_ = nullptr;
    lv_obj_t* left_arc_ = nullptr;
    lv_obj_t* right_arc_ = nullptr;
    lv_obj_t* left_brow_ = nullptr;
    lv_obj_t* right_brow_ = nullptr;
    lv_obj_t* mouth_ = nullptr;
    lv_obj_t* tongue_ = nullptr;
    lv_obj_t* left_tear_ = nullptr;
    lv_obj_t* right_tear_ = nullptr;

    std::array<lv_obj_t*, kChromeObjectCount> chrome_objects_{};
    size_t chrome_count_ = 0;
    std::array<lv_point_precise_t, 2> left_line_points_{};
    std::array<lv_point_precise_t, 2> right_line_points_{};
    Expression current_expression_ = Expression::NORMAL;
    Expression target_expression_ = Expression::NORMAL;
    uint32_t transition_elapsed_ms_ = 0;
    uint32_t transition_duration_ms_ = 240;
    uint32_t blink_due_ms_ = 0;
    uint32_t blink_elapsed_ms_ = 0;
    uint32_t speech_elapsed_ms_ = 0;
    uint32_t eye_motion_elapsed_ms_ = 0;
    bool blinking_ = false;
    bool fullscreen_ = false;
        bool speaking_mouth_animation_ = true;
        int animation_speed_percent_ = 100;
    uint32_t random_state_ = 0x6d6f6368;
    lv_timer_t* timer_ = nullptr;
    static MochiFaceController* active_controller_;
};

#endif