#include "mochi_face_controller.h"

#include <algorithm>
#include <cstring>
#include <cmath>

namespace {
constexpr lv_coord_t kEyeWidth = 70;
constexpr lv_coord_t kEyeHeight = 92;
constexpr lv_coord_t kStroke = 8;
const lv_color_t kFaceColor = lv_color_white();

uint32_t NextRandom(uint32_t& state) {
    state = state * 1664525u + 1013904223u;
    return state;
}
}

MochiFaceController* MochiFaceController::active_controller_ = nullptr;

MochiFaceController::MochiFaceController(lv_obj_t* parent) {
    lv_obj_t* face_parent = parent != nullptr ? parent : lv_screen_active();
    lv_obj_update_layout(face_parent);
    root_ = lv_obj_create(face_parent);
    lv_coord_t parent_width = lv_obj_get_width(face_parent);
    lv_coord_t parent_height = lv_obj_get_height(face_parent);
    if (parent_width <= 0) parent_width = lv_display_get_horizontal_resolution(lv_display_get_default());
    if (parent_height <= 0) parent_height = lv_display_get_vertical_resolution(lv_display_get_default());
    lv_obj_set_size(root_, parent_width, parent_height);
    lv_obj_align(root_, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_bg_color(root_, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(root_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(root_, 0, 0);
    lv_obj_set_style_pad_all(root_, 0, 0);
    lv_obj_clear_flag(root_, static_cast<lv_obj_flag_t>(LV_OBJ_FLAG_SCROLLABLE));
    lv_obj_clear_flag(root_, static_cast<lv_obj_flag_t>(LV_OBJ_FLAG_CLICKABLE));
    lv_obj_add_flag(root_, LV_OBJ_FLAG_HIDDEN);
    CreateFaceObjects();
    blink_due_ms_ = 2600 + (NextRandom(random_state_) % 4200);
    active_controller_ = this;
    timer_ = lv_timer_create(TimerCallback, 50, nullptr);
    lv_timer_pause(timer_);
    Render();
}

MochiFaceController::~MochiFaceController() {
    if (timer_ != nullptr) lv_timer_delete(timer_);
    if (active_controller_ == this) active_controller_ = nullptr;
    if (root_ != nullptr) lv_obj_del(root_);
}

void MochiFaceController::CreateFaceObjects() {
    left_eye_ = lv_obj_create(root_);
    right_eye_ = lv_obj_create(root_);
    left_pupil_ = lv_obj_create(root_);
    right_pupil_ = lv_obj_create(root_);
    left_arc_ = lv_arc_create(root_);
    right_arc_ = lv_arc_create(root_);
    left_brow_ = lv_line_create(root_);
    right_brow_ = lv_line_create(root_);
    mouth_ = lv_line_create(root_);
    tongue_ = lv_obj_create(root_);
    left_tear_ = lv_obj_create(root_);
    right_tear_ = lv_obj_create(root_);

    for (lv_obj_t* object : {left_eye_, right_eye_, left_pupil_, right_pupil_, tongue_, left_tear_, right_tear_}) {
        lv_obj_set_style_bg_color(object, kFaceColor, 0);
        lv_obj_set_style_border_width(object, 0, 0);
        lv_obj_set_style_radius(object, LV_RADIUS_CIRCLE, 0);
    }
    for (lv_obj_t* arc : {left_arc_, right_arc_}) {
        lv_obj_set_style_arc_color(arc, kFaceColor, LV_PART_MAIN);
        lv_obj_set_style_arc_width(arc, kStroke, LV_PART_MAIN);
        lv_obj_set_style_arc_opa(arc, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_arc_opa(arc, LV_OPA_TRANSP, LV_PART_INDICATOR);
        lv_obj_remove_style(arc, nullptr, LV_PART_KNOB);
    }
    for (lv_obj_t* line : {left_brow_, right_brow_}) {
        lv_obj_set_style_line_color(line, kFaceColor, 0);
        lv_obj_set_style_line_width(line, kStroke, 0);
        lv_obj_set_style_line_rounded(line, true, 0);
    }
    lv_obj_set_style_line_color(mouth_, kFaceColor, 0);
    lv_obj_set_style_line_width(mouth_, kStroke, 0);
    lv_obj_set_style_line_rounded(mouth_, true, 0);
    lv_obj_add_flag(tongue_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_bg_color(left_pupil_, lv_color_black(), 0);
    lv_obj_set_style_bg_color(right_pupil_, lv_color_black(), 0);
    lv_obj_set_size(left_pupil_, 12, 16);
    lv_obj_set_size(right_pupil_, 12, 16);
    lv_obj_add_flag(left_tear_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(right_tear_, LV_OBJ_FLAG_HIDDEN);
}

void MochiFaceController::SetExpression(Expression expression, uint32_t transition_ms) {
    target_expression_ = expression;
    transition_elapsed_ms_ = 0;
    transition_duration_ms_ = std::max<uint32_t>(1, transition_ms);
    Refresh();
}

void MochiFaceController::SetExpression(const char* expression, uint32_t transition_ms) {
    SetExpression(ParseExpression(expression), transition_ms);
}

void MochiFaceController::SetAnimationSpeed(int speed_percent) {
    animation_speed_percent_ = std::max(50, std::min(200, speed_percent));
    Refresh();
}

void MochiFaceController::Refresh() {
    Render();
    lv_obj_invalidate(root_);
}

void MochiFaceController::Update(uint32_t elapsed_ms) {
    lv_obj_update_layout(root_);
    if (current_expression_ != target_expression_) {
        transition_elapsed_ms_ = std::min(transition_elapsed_ms_ + elapsed_ms, transition_duration_ms_);
        if (transition_elapsed_ms_ == transition_duration_ms_) current_expression_ = target_expression_;
    }

    if (blink_due_ms_ <= elapsed_ms) {
        blinking_ = true;
        blink_elapsed_ms_ = 0;
        blink_due_ms_ = 2600 + (NextRandom(random_state_) % 4200);
    } else {
        blink_due_ms_ -= elapsed_ms;
    }
    if (blinking_) {
        blink_elapsed_ms_ += elapsed_ms;
        if (blink_elapsed_ms_ >= 180) {
            blinking_ = false;
            blink_elapsed_ms_ = 0;
        }
    }
    speech_elapsed_ms_ += elapsed_ms;
    eye_motion_elapsed_ms_ += elapsed_ms;
    Refresh();
}

void MochiFaceController::Render() {
    lv_obj_t* parent = lv_obj_get_parent(root_);
    if (parent != nullptr) {
        lv_obj_update_layout(parent);
        const lv_coord_t parent_width = lv_obj_get_width(parent);
        const lv_coord_t parent_height = lv_obj_get_height(parent);
        if (parent_width > 0 && parent_height > 0) {
            lv_obj_set_size(root_, parent_width, parent_height);
        }
    }
    lv_obj_update_layout(root_);
    const lv_coord_t display_width = lv_obj_get_width(root_);
    const lv_coord_t display_height = lv_obj_get_height(root_);
    const float scale = std::max(0.25f, std::min(display_width / 210.0f, display_height / 150.0f));
    const lv_coord_t eye_width = static_cast<lv_coord_t>(kEyeWidth * scale);
    const lv_coord_t eye_height_base = static_cast<lv_coord_t>(kEyeHeight * scale);
    const lv_coord_t stroke = std::max<lv_coord_t>(3, static_cast<lv_coord_t>(kStroke * scale));
    const float transition = current_expression_ == target_expression_
        ? 1.0f
        : static_cast<float>(transition_elapsed_ms_) / transition_duration_ms_;
    const bool blink_closed = blinking_ && blink_elapsed_ms_ >= 55 && blink_elapsed_ms_ < 125;
    const bool happy = target_expression_ == Expression::HAPPY || target_expression_ == Expression::LAUGHING || current_expression_ == Expression::HAPPY || current_expression_ == Expression::LAUGHING;
    const bool sleepy = target_expression_ == Expression::SLEEPY || current_expression_ == Expression::SLEEPY;
    const bool cry = target_expression_ == Expression::CRYING || target_expression_ == Expression::SAD || current_expression_ == Expression::CRYING || current_expression_ == Expression::SAD;
    const bool silly = target_expression_ == Expression::SILLY || target_expression_ == Expression::LOVE || current_expression_ == Expression::SILLY || current_expression_ == Expression::LOVE;
    const bool angry = target_expression_ == Expression::ANGRY || current_expression_ == Expression::ANGRY;
    const bool surprised = target_expression_ == Expression::SURPRISED || current_expression_ == Expression::SURPRISED;
    const bool thinking = target_expression_ == Expression::THINKING || target_expression_ == Expression::CONFUSED || current_expression_ == Expression::THINKING || current_expression_ == Expression::CONFUSED;
    const bool listening = target_expression_ == Expression::LISTENING || current_expression_ == Expression::LISTENING;
    const bool speaking = target_expression_ == Expression::SPEAKING || current_expression_ == Expression::SPEAKING;
    const lv_coord_t eye_height = (blink_closed || sleepy) ? std::max<lv_coord_t>(2, static_cast<lv_coord_t>(4 * scale)) : eye_height_base;
    const lv_coord_t eye_motion_x = static_cast<lv_coord_t>(std::sin(eye_motion_elapsed_ms_ / 900.0f) * 5 * scale);
    const lv_coord_t eye_motion_y = static_cast<lv_coord_t>(std::sin(eye_motion_elapsed_ms_ / 1300.0f) * 3 * scale);

    for (lv_obj_t* eye : {left_eye_, right_eye_}) {
        lv_obj_set_size(eye, eye_width, eye_height);
        lv_obj_set_style_radius(eye, eye_height <= 4 * scale ? 2 : LV_RADIUS_CIRCLE, 0);
    }
    const lv_coord_t eye_offset_x = static_cast<lv_coord_t>(70 * scale);
    const lv_coord_t eye_offset_y = static_cast<lv_coord_t>((surprised ? -28 : -22) * scale);
    lv_obj_align(left_eye_, LV_ALIGN_CENTER, -eye_offset_x, eye_offset_y);
    lv_obj_align(right_eye_, LV_ALIGN_CENTER, eye_offset_x, eye_offset_y);
    const lv_coord_t pupil_width = std::max<lv_coord_t>(5, static_cast<lv_coord_t>(18 * scale));
    const lv_coord_t pupil_height = std::max<lv_coord_t>(6, static_cast<lv_coord_t>(26 * scale));
    for (lv_obj_t* pupil : {left_pupil_, right_pupil_}) {
        lv_obj_set_size(pupil, pupil_width, pupil_height);
        if (blink_closed || sleepy) lv_obj_add_flag(pupil, LV_OBJ_FLAG_HIDDEN);
        else lv_obj_clear_flag(pupil, LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_align(left_pupil_, LV_ALIGN_CENTER, -eye_offset_x + eye_motion_x, eye_offset_y + eye_motion_y);
    lv_obj_align(right_pupil_, LV_ALIGN_CENTER, eye_offset_x + eye_motion_x, eye_offset_y + eye_motion_y);

    const bool current_happy = current_expression_ == Expression::HAPPY || current_expression_ == Expression::LAUGHING;
    const bool current_angry = current_expression_ == Expression::ANGRY;
    const bool current_cry = current_expression_ == Expression::CRYING || current_expression_ == Expression::SAD;
    const bool current_silly = current_expression_ == Expression::SILLY || current_expression_ == Expression::LOVE;
    const lv_opa_t arc_opa = happy ? static_cast<lv_opa_t>(255 * (target_expression_ == Expression::HAPPY ? transition : 1.0f - transition)) : 0;
    const lv_opa_t brow_opa = angry ? static_cast<lv_opa_t>(255 * (target_expression_ == Expression::ANGRY ? transition : 1.0f - transition)) : 0;
    const lv_opa_t tear_opa = cry ? static_cast<lv_opa_t>(255 * (target_expression_ == Expression::CRYING || target_expression_ == Expression::SAD ? transition : 1.0f - transition)) : 0;
    const lv_opa_t tongue_opa = silly ? static_cast<lv_opa_t>(255 * (target_expression_ == Expression::SILLY ? transition : 1.0f - transition)) : 0;
    const bool show_arcs = (happy || current_happy) && !blink_closed;
    for (lv_obj_t* arc : {left_arc_, right_arc_}) {
        lv_obj_set_size(arc, eye_width + static_cast<lv_coord_t>(12 * scale), eye_height_base + static_cast<lv_coord_t>(12 * scale));
        if (show_arcs) {
            lv_obj_clear_flag(arc, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_opa(arc, arc_opa, 0);
            lv_arc_set_angles(arc, 205, 335);
        } else {
            lv_obj_add_flag(arc, LV_OBJ_FLAG_HIDDEN);
        }
    }
    lv_obj_align(left_arc_, LV_ALIGN_CENTER, -eye_offset_x, eye_offset_y);
    lv_obj_align(right_arc_, LV_ALIGN_CENTER, eye_offset_x, eye_offset_y);

    left_line_points_[0] = {0, static_cast<lv_coord_t>((angry ? 12 : 6) * scale)};
    left_line_points_[1] = {eye_width, static_cast<lv_coord_t>((angry ? 0 : 6) * scale)};
    right_line_points_[0] = {0, static_cast<lv_coord_t>((angry ? 0 : 6) * scale)};
    right_line_points_[1] = {eye_width, static_cast<lv_coord_t>((angry ? 12 : 6) * scale)};
    lv_line_set_points(left_brow_, left_line_points_.data(), 2);
    lv_line_set_points(right_brow_, right_line_points_.data(), 2);
    lv_obj_set_style_line_width(left_brow_, stroke, 0);
    lv_obj_set_style_line_width(right_brow_, stroke, 0);
    lv_obj_set_size(left_brow_, eye_width, static_cast<lv_coord_t>(18 * scale));
    lv_obj_set_size(right_brow_, eye_width, static_cast<lv_coord_t>(18 * scale));
    lv_obj_align(left_brow_, LV_ALIGN_CENTER, -static_cast<lv_coord_t>(79 * scale), -static_cast<lv_coord_t>(64 * scale));
    lv_obj_align(right_brow_, LV_ALIGN_CENTER, static_cast<lv_coord_t>(37 * scale), -static_cast<lv_coord_t>(64 * scale));
    for (lv_obj_t* brow : {left_brow_, right_brow_}) {
        if (angry || current_angry) {
            lv_obj_clear_flag(brow, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_opa(brow, brow_opa, 0);
        }
        else lv_obj_add_flag(brow, LV_OBJ_FLAG_HIDDEN);
    }

    const uint32_t speech_period = static_cast<uint32_t>(115 * 100 / animation_speed_percent_);
    const uint32_t speech_phase = (speech_elapsed_ms_ / std::max<uint32_t>(1, speech_period)) % 5;
    const lv_coord_t speech_widths[] = {18, 27, 36, 29, 22};
    const lv_coord_t speech_heights[] = {5, 10, 17, 12, 7};
    const bool animate_speaking = speaking && speaking_mouth_animation_;
    const lv_coord_t mouth_width = animate_speaking ? speech_widths[speech_phase] : (silly ? 34 : (cry ? 26 : thinking ? 18 : 30));
    const lv_coord_t mouth_height = animate_speaking ? speech_heights[speech_phase] : (silly ? 20 : surprised ? 18 : 6);
    const lv_coord_t mouth_depth = static_cast<lv_coord_t>((animate_speaking ? mouth_height : (surprised ? 18 : 12)) * scale);
    const lv_coord_t mouth_width_scaled = static_cast<lv_coord_t>(mouth_width * scale);
    const lv_coord_t mouth_stroke = std::max<lv_coord_t>(2, static_cast<lv_coord_t>(7 * scale));
    const lv_coord_t mouth_x_step = mouth_width_scaled / 4;
    mouth_line_points_[0] = {mouth_stroke, mouth_stroke};
    mouth_line_points_[1] = {static_cast<lv_coord_t>(mouth_stroke + mouth_x_step), static_cast<lv_coord_t>(mouth_stroke + mouth_depth * 0.7f)};
    mouth_line_points_[2] = {static_cast<lv_coord_t>(mouth_stroke + mouth_x_step * 2), static_cast<lv_coord_t>(mouth_stroke + mouth_depth)};
    mouth_line_points_[3] = {static_cast<lv_coord_t>(mouth_stroke + mouth_x_step * 3), static_cast<lv_coord_t>(mouth_stroke + mouth_depth * 0.7f)};
    mouth_line_points_[4] = {static_cast<lv_coord_t>(mouth_stroke + mouth_width_scaled), mouth_stroke};
    lv_line_set_points(mouth_, mouth_line_points_.data(), mouth_line_points_.size());
    lv_obj_set_style_line_width(mouth_, mouth_stroke, 0);
    lv_obj_set_size(mouth_, mouth_width_scaled + mouth_stroke * 2, mouth_depth + static_cast<lv_coord_t>(10 * scale) + mouth_stroke * 2);
    lv_obj_align(mouth_, LV_ALIGN_CENTER, listening ? static_cast<lv_coord_t>(-8 * scale) : 0, static_cast<lv_coord_t>((70 + (listening ? 4 : 0)) * scale));
    if (silly || current_silly) {
        lv_obj_set_size(tongue_, static_cast<lv_coord_t>(14 * scale), static_cast<lv_coord_t>(18 * scale));
        lv_obj_align(tongue_, LV_ALIGN_CENTER, 0, static_cast<lv_coord_t>(78 * scale));
        lv_obj_clear_flag(tongue_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_opa(tongue_, tongue_opa, 0);
    } else {
        lv_obj_add_flag(tongue_, LV_OBJ_FLAG_HIDDEN);
    }

    for (lv_obj_t* tear : {left_tear_, right_tear_}) {
        lv_obj_set_size(tear, static_cast<lv_coord_t>(12 * scale), static_cast<lv_coord_t>(22 * scale));
        lv_obj_set_style_radius(tear, LV_RADIUS_CIRCLE, 0);
    }
    lv_obj_align(left_tear_, LV_ALIGN_CENTER, -eye_offset_x, static_cast<lv_coord_t>(25 * scale));
    lv_obj_align(right_tear_, LV_ALIGN_CENTER, eye_offset_x, static_cast<lv_coord_t>(25 * scale));
    if (cry || current_cry) {
        lv_obj_clear_flag(left_tear_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(right_tear_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_opa(left_tear_, tear_opa, 0);
        lv_obj_set_style_opa(right_tear_, tear_opa, 0);
    } else {
        lv_obj_add_flag(left_tear_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(right_tear_, LV_OBJ_FLAG_HIDDEN);
    }
}

void MochiFaceController::RegisterChromeObject(lv_obj_t* object) {
    if (object != nullptr && chrome_count_ < chrome_objects_.size()) chrome_objects_[chrome_count_++] = object;
}

void MochiFaceController::AnimateOpacity(lv_obj_t* object, uint8_t from, uint8_t to, uint32_t duration_ms, bool hide_at_end) {
    lv_anim_t animation;
    lv_anim_init(&animation);
    lv_anim_set_var(&animation, object);
    lv_anim_set_values(&animation, from, to);
    lv_anim_set_duration(&animation, duration_ms);
    lv_anim_set_exec_cb(&animation, [](void* variable, int32_t value) {
        lv_obj_set_style_opa(static_cast<lv_obj_t*>(variable), static_cast<lv_opa_t>(value), 0);
    });
    if (hide_at_end) lv_anim_set_completed_cb(&animation, HideAnimationComplete);
    lv_anim_start(&animation);
}

void MochiFaceController::HideAnimationComplete(lv_anim_t* animation) {
    lv_obj_add_flag(static_cast<lv_obj_t*>(animation->var), LV_OBJ_FLAG_HIDDEN);
}

void MochiFaceController::TimerCallback(lv_timer_t* timer) {
    (void)timer;
    if (active_controller_ != nullptr) active_controller_->Update(50);
}

void MochiFaceController::EnterFullscreenFace(uint32_t duration_ms) {
    (void)duration_ms;
    fullscreen_ = true;
    lv_timer_resume(timer_);
    lv_obj_set_style_bg_color(root_, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(root_, LV_OPA_COVER, 0);
    lv_obj_clear_flag(root_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(root_);
    for (size_t index = 0; index < chrome_count_; ++index) {
        if (chrome_objects_[index] != nullptr) {
            chrome_was_hidden_[index] = lv_obj_has_flag(chrome_objects_[index], LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(chrome_objects_[index], LV_OBJ_FLAG_HIDDEN);
        }
    }
    Render();
    lv_obj_invalidate(root_);
}

void MochiFaceController::ExitFullscreenFace(uint32_t duration_ms) {
    (void)duration_ms;
    fullscreen_ = false;
    lv_timer_pause(timer_);
    for (size_t index = 0; index < chrome_count_; ++index) {
        if (chrome_objects_[index] != nullptr) {
            if (chrome_was_hidden_[index]) {
                lv_obj_add_flag(chrome_objects_[index], LV_OBJ_FLAG_HIDDEN);
            } else {
                lv_obj_clear_flag(chrome_objects_[index], LV_OBJ_FLAG_HIDDEN);
            }
        }
    }
    lv_obj_add_flag(root_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_invalidate(lv_screen_active());
}

MochiFaceController::Expression MochiFaceController::ParseExpression(const char* expression) {
    if (expression == nullptr) return Expression::NORMAL;
    if (strcmp(expression, "normal") == 0 || strcmp(expression, "neutral") == 0) return Expression::NORMAL;
    if (strcmp(expression, "happy") == 0) return Expression::HAPPY;
    if (strcmp(expression, "angry") == 0) return Expression::ANGRY;
    if (strcmp(expression, "cry") == 0 || strcmp(expression, "crying") == 0) return Expression::CRYING;
    if (strcmp(expression, "sad") == 0) return Expression::SAD;
    if (strcmp(expression, "silly") == 0 || strcmp(expression, "funny") == 0) return Expression::SILLY;
    if (strcmp(expression, "surprised") == 0 || strcmp(expression, "surprise") == 0) return Expression::SURPRISED;
    if (strcmp(expression, "sleepy") == 0) return Expression::SLEEPY;
    if (strcmp(expression, "love") == 0) return Expression::LOVE;
    if (strcmp(expression, "confused") == 0) return Expression::CONFUSED;
    if (strcmp(expression, "laughing") == 0 || strcmp(expression, "laugh") == 0) return Expression::LAUGHING;
    if (strcmp(expression, "thinking") == 0 || strcmp(expression, "think") == 0) return Expression::THINKING;
    if (strcmp(expression, "listening") == 0) return Expression::LISTENING;
    if (strcmp(expression, "speaking") == 0) return Expression::SPEAKING;
    return Expression::NORMAL;
}