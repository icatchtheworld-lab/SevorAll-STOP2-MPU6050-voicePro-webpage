#include "hal_data.h"
#include "app.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "drv_uart.h"
#include "Servor.h"
#include "VoiceControl.h"
#include "SensorMPU6050.h"
#include "Flash.h"

#include "Wifi.h"

#include <math.h>


#define MPU_CTRL_PERIOD_MS           (50U)//定时50ms    主循环周期，单位为毫秒
#define MPU_DEBUG_PERIOD_MS          (500U)//目标时间

#define ROT_CENTER_DEG               (90)//初始角度
#define ROT_MIN_DEG                  (0)//安全角度min
#define ROT_MAX_DEG                  (180)//安全角度max
#define ROT_STEP_DEG                 (2U)

#define EXT_COMMON_CENTER_DEG        (90)//共有度数 在这个基础上进行偏移操作

#define EXT_A_CENTER_DEG             (90)//初始角度
#define EXT_A_MIN_DEG                (0)//安全角度min
#define EXT_A_MAX_DEG                (180)//安全角度max
#define EXT_A_STEP_DEG               (2U)

#define EXT_C_CENTER_DEG             (90)//初始角度
#define EXT_C_MIN_DEG                (0)//安全角度min
#define EXT_C_MAX_DEG                (180)//安全角度max
#define EXT_C_STEP_DEG               (1U)

#define EXT_D_CENTER_DEG             (90)//初始角度
#define EXT_D_MIN_DEG                (0)//安全角度min
#define EXT_D_MAX_DEG                (180)//安全角度max
#define EXT_D_STEP_DEG               (2U)

#define EXT_DEG_QUANT                (2)//过滤小于2°的变化

#define EXT_A_DIR                    (-1)//需要反转
#define EXT_C_DIR                    (1)//不需要反转
#define EXT_D_DIR                    (1)//不需要反转

#define EXT_A_GAIN_NUM               (1)//调整舵机的转动灵敏度
#define EXT_A_GAIN_DEN               (1)//调整舵机的转动灵敏度

#define EXT_C_GAIN_NUM               (1)
#define EXT_C_GAIN_DEN               (1)

#define EXT_D_GAIN_NUM               (1)
#define EXT_D_GAIN_DEN               (1)

#define AX_DEADZONE_COUNTS           (2500)//模拟量抖动
#define AY_DEADZONE_COUNTS           (1500)//模拟量抖动
#define AX_DIV_COUNTS_PER_DEG        (200)//x轴灵敏度系数
#define AY_DIV_COUNTS_PER_DEG        (200)//y轴灵敏度系数

#define VOICE_EXT_A_TARGET_DEG       (90U)//A舵机目标角度
#define VOICE_EXT_D_TARGET_DEG_MIN      (0U)//D舵机目标角度
#define VOICE_EXT_D_TARGET_DEG_MAX     (180)//舵机D另一个目标
#define VOICE_EXT_A_STEP_DEG           (16U)
#define VOICE_EXT_D_STEP_DEG           (10U)

#define ACTION_MAX_FRAMES              (300U)//存储最大动作帧



FSP_CPP_HEADER
void R_BSP_WarmStart(bsp_warm_start_event_t event);
FSP_CPP_FOOTER

void mpu_i2c_callback(i2c_master_callback_args_t * p_args)
{
    MPU6050_I2C_EventSet(p_args->event);
}

typedef struct st_mpu_att_state
{
    float roll_deg;
    float pitch_deg;
    float alpha;
    int32_t gx_bias_raw;
    int32_t gy_bias_raw;
} mpu_att_state_t;

static void mpu_att_init(mpu_att_state_t * st)
{
    if (NULL == st)
    {
        return;
    }
    st->roll_deg = 0.0f;
    st->pitch_deg = 0.0f;
    st->alpha = 0.98f;
    st->gx_bias_raw = 0;
    st->gy_bias_raw = 0;
}

static void mpu_att_update(mpu_att_state_t * st,
                           int32_t ax,
                           int32_t ay,
                           int32_t az,
                           int32_t gx_raw,
                           int32_t gy_raw,
                           float dt_s,
                           float * out_roll_deg,
                           float * out_pitch_deg)
{
    if ((NULL == st) || (NULL == out_roll_deg) || (NULL == out_pitch_deg))
    {
        return;
    }

    float axf = (float) ax;
    float ayf = (float) ay;
    float azf = (float) az;

    float roll_acc = atan2f(ayf, azf) * (180.0f / 3.1415926f);
    float pitch_acc = atan2f(-axf, sqrtf((ayf * ayf) + (azf * azf))) * (180.0f / 3.1415926f);

    float gx_dps = ((float) (gx_raw - st->gx_bias_raw)) / 131.0f;
    float gy_dps = ((float) (gy_raw - st->gy_bias_raw)) / 131.0f;

    float roll_gyro = st->roll_deg + (gx_dps * dt_s);
    float pitch_gyro = st->pitch_deg + (gy_dps * dt_s);

    st->roll_deg = (st->alpha * roll_gyro) + ((1.0f - st->alpha) * roll_acc);
    st->pitch_deg = (st->alpha * pitch_gyro) + ((1.0f - st->alpha) * pitch_acc);

    *out_roll_deg = st->roll_deg;
    *out_pitch_deg = st->pitch_deg;
}

static int32_t iabs32(int32_t v)
{
    return (v < 0) ? -v : v;
}

static uint32_t clamp_i32_to_u32(int32_t v, uint32_t min_v, uint32_t max_v)
{
    if (v < (int32_t) min_v)
    {
        return min_v;
    }
    if (v > (int32_t) max_v)
    {
        return max_v;
    }
    return (uint32_t) v;
}

static uint32_t step_towards_u32(uint32_t current, uint32_t target, uint32_t step)
{
    //如果当前角度小于目标角度
    if (current < target)
    {
        //则每次加step
        uint32_t next = current + step;
        //如果next溢出目标角度 则直接返回目标角度
        return (next > target) ? target : next;
    }
    //如果目标角度小于当前角度
    if (current > target)
    {//current > step防止下溢  如果不够减1°那么直接将目标角度设置为0°
        uint32_t next = (current > step) ? (current - step) : 0U;
        return (next < target) ? target : next;
    }
    return current;
}

typedef enum
{
    CONTROL_MODE_WIRED = 0,
    CONTROL_MODE_WIRELESS = 1,
} control_mode_t;

static control_mode_t g_control_mode = CONTROL_MODE_WIRED;//初始g_control_mode为有线控制

static int wifi_try_parse_servo_cmd(char const * s, char * out_servo, uint32_t * out_deg)
{
    if ((NULL == s) || (NULL == out_servo) || (NULL == out_deg))
    {
        return 0;
    }

    char servo = s[0];
    if ((servo >= 'a') && (servo <= 'z'))
    {
        servo = (char) (servo - 'a' + 'A');
    }

    if ((servo != 'A') && (servo != 'B') && (servo != 'C') && (servo != 'D') && (servo != 'E'))
    {
        return 0;
    }

    if (s[1] != ':')
    {
        return 0;
    }

    char const * p = &s[2];
    if ((*p < '0') || (*p > '9'))
    {
        return 0;
    }

    char * endptr = NULL;
    unsigned long v = strtoul(p, &endptr, 10);
    if ((endptr == p) || (v > 180UL))
    {
        return 0;
    }

    if ((*endptr != '\0') && (*endptr != '\r') && (*endptr != '\n'))
    {
        return 0;
    }
    while ((*endptr == '\r') || (*endptr == '\n'))
    {
        endptr++;
    }
    if (*endptr != '\0')
    {
        return 0;
    }

    *out_servo = servo;
    *out_deg = (uint32_t) v;
    return 1;
}

static int http_try_parse_set_query(char const * path, char servo_letter, uint32_t * out_deg)
{
    if ((NULL == path) || (NULL == out_deg))
    {
        return 0;
    }

    char key[3];
    key[0] = servo_letter;
    key[1] = '=';
    key[2] = '\0';

    char const * q = strchr(path, '?');
    if (NULL == q)
    {
        return 0;
    }
    q++;

    char const * p = q;
    while (*p != '\0' && *p != ' ' && *p != '\r' && *p != '\n')
    {
        if (((p == q) || (p[-1] == '&')) && (p[0] == key[0]) && (p[1] == '='))
        {
            char * endptr = NULL;
            unsigned long v = strtoul(&p[2], &endptr, 10);
            if ((endptr == &p[2]) || (v > 180UL))
            {
                return 0;
            }
            *out_deg = (uint32_t) v;
            return 1;
        }

        char const * amp = strchr(p, '&');
        if (NULL == amp)
        {
            break;
        }
        p = amp + 1;
    }

    return 0;
}

static void wifi_http_send(uint32_t socket_id, char const * s)
{
    if ((0U == socket_id) || (NULL == s))
    {
        return;
    }
    uint32_t len = (uint32_t) strlen(s);
    uint32_t off = 0U;
    while (off < len)
    {
        uint32_t chunk = len - off;
        if (chunk > 128U)
        {
            chunk = 128U;
        }
        int ret = Wifi_TcpSend(socket_id, (uint8_t const *) &s[off], chunk);
        if (ret != 0)
        {
            printf("HTTP send err=%d off=%lu len=%lu\r\n", ret, (unsigned long) off, (unsigned long) len);
            break;
        }
        off += chunk;
        R_BSP_SoftwareDelay(10U, BSP_DELAY_UNITS_MILLISECONDS);
    }
}

static void wifi_http_send_html_page(uint32_t socket_id)
{
    static const char html[] =
        "<!doctype html><html><head><meta name=viewport content=\"width=device-width,initial-scale=1\">"
        "<title>Servo Control</title>"
        "<style>body{font-family:system-ui,Arial;margin:16px} .row{margin:14px 0} label{display:block;margin-bottom:6px} input[type=range]{width:100%} .v{font-variant-numeric:tabular-nums}</style>"
        "</head><body>"
        "<h2>WiFi Servo Control</h2>"
        "<div class=row><label>A: <span id=va class=v>90</span></label><input id=a type=range min=0 max=180 value=90></div>"
        "<div class=row><label>B: <span id=vb class=v>90</span></label><input id=b type=range min=0 max=180 value=90></div>"
        "<div class=row><label>C: <span id=vc class=v>90</span></label><input id=c type=range min=0 max=180 value=90></div>"
        "<div class=row><label>D: <span id=vd class=v>90</span></label><input id=d type=range min=0 max=180 value=90></div>"
        "<div class=row><label>E: <span id=ve class=v>90</span></label><input id=e type=range min=0 max=180 value=90></div>"
        "<script>"
        "const $=id=>document.getElementById(id);"
        "const st={A:90,B:90,C:90,D:90,E:90};"
        "let t=0;"
        "function send(){clearTimeout(t);t=setTimeout(()=>{"
        "const q=`/set?A=${st.A}&B=${st.B}&C=${st.C}&D=${st.D}&E=${st.E}`;"
        "fetch(q,{cache:'no-store'}).catch(()=>{});"
        "},80);}"
        "function bind(k,slider,span){slider.addEventListener('input',()=>{st[k]=parseInt(slider.value,10)||0;span.textContent=st[k];send();});}"
        "bind('A',$('a'),$('va'));bind('B',$('b'),$('vb'));bind('C',$('c'),$('vc'));bind('D',$('d'),$('vd'));bind('E',$('e'),$('ve'));"
        "</script></body></html>";

    char hdr[160];
    int hn = snprintf(hdr, sizeof(hdr),
                      "HTTP/1.1 200 OK\r\n"
                      "Content-Type: text/html; charset=utf-8\r\n"
                      "Cache-Control: no-store\r\n"
                      "Content-Length: %lu\r\n"
                      "Connection: keep-alive\r\n"
                      "\r\n",
                      (unsigned long) strlen(html));
    if (hn > 0)
    {
        wifi_http_send(socket_id, hdr);
        wifi_http_send(socket_id, html);
    }
}

static void handle_voice_mem_events(action_frame_t * frames,
                                   uint32_t max_frames,
                                   uint32_t * p_frame_count,
                                   uint8_t  * p_recording,
                                   uint8_t  * p_playing,
                                   uint32_t * p_play_idx)
{
    if (voice_mem_start_try_consume())
    {
        *p_recording = 1U;
        *p_playing = 0U;
        *p_play_idx = 0U;//复现索引置为0，清空之前的复现进度
        *p_frame_count = 0U;//把「动作帧计数」重置为 0，清空之前的记录
    }

    if (voice_mem_end_try_consume())
    {
        *p_recording = 0U;
        if (*p_frame_count > 0U)
        {
            (void) action_flash_save(frames, *p_frame_count);
        }
    }

    if (voice_mem_play_try_consume())
    {
        uint32_t loaded = 0U;
        if (FSP_SUCCESS == action_flash_load(frames, max_frames, &loaded))
        {
            *p_frame_count = loaded;
            *p_playing = (loaded > 0U) ? 1U : 0U;
            *p_play_idx = 0U;
        }
    }
}

static uint8_t playback_step(action_frame_t const * frames, uint32_t frame_count, uint32_t * p_idx)
{
    if (*p_idx >= frame_count)
    {
        return 0U;
    }

    action_frame_t const * f = &frames[*p_idx];
    servo_set_angleA((uint32_t) f->a);
    servo_set_angleB((uint32_t) f->b);
    servo_set_angleC((uint32_t) f->c);
    servo_set_angleD((uint32_t) f->d);

    uint32_t e_target = (uint32_t) f->e;
    uint32_t e_cur = servo_get_angleE();

    uint32_t remaining_ms = (uint32_t) f->dt_ms;
    while (remaining_ms > 0U)
    {
        if (servo_emergency_stop_is_set())
        {
            break;
        }

        uint32_t e_next = step_towards_u32(e_cur, e_target, 2U);
        if (e_next != e_cur)
        {
            e_cur = e_next;
            servo_set_angleE(e_cur);
        }

        uint32_t slice = (remaining_ms > 10U) ? 10U : remaining_ms;
        R_BSP_SoftwareDelay(slice, BSP_DELAY_UNITS_MILLISECONDS);
        remaining_ms -= slice;

        if (e_cur == e_target)
        {
            if (remaining_ms > 0U)
            {
                R_BSP_SoftwareDelay(remaining_ms, BSP_DELAY_UNITS_MILLISECONDS);
            }
            break;
        }
    }

    (*p_idx)++;
    return 1U;
}

static void record_frame_if_enabled(uint8_t recording,//录制开关
                                   action_frame_t * frames,//动作帧结构体指针
                                   uint32_t max_frames,//最大存储帧
                                   uint32_t * p_frame_count,//已录制的帧数
                                   uint32_t a_deg,
                                   uint32_t c_deg,
                                   uint32_t d_deg,
                                   uint32_t e_deg,
                                   uint16_t dt_ms)//时间间隔
{
    if (!recording)
    {
        return;
    }

    if (*p_frame_count >= max_frames)
    {
        return;
    }
    //创建结构体指针
    action_frame_t * f = &frames[(*p_frame_count)++];
    f->dt_ms = dt_ms;
    f->a = (uint8_t) a_deg;
    f->b = (uint8_t) servo_get_angleB();
    f->c = (uint8_t) c_deg;
    f->d = (uint8_t) d_deg;
    f->e = (uint8_t) e_deg;
    f->reserved = 0U;
}

static void voice_extend_handle_steps(uint32_t * p_ext_a_deg, uint32_t * p_ext_d_deg)
{
    while (voice_extend_step_try_consume())
    {
        if (*p_ext_a_deg > VOICE_EXT_A_TARGET_DEG)
        {
            uint32_t next_a = *p_ext_a_deg - VOICE_EXT_A_STEP_DEG;
            *p_ext_a_deg = (next_a < VOICE_EXT_A_TARGET_DEG) ? VOICE_EXT_A_TARGET_DEG : next_a;
            servo_set_angleA(*p_ext_a_deg);
        }
        if (*p_ext_a_deg < VOICE_EXT_A_TARGET_DEG)
        {
            uint32_t next_a = *p_ext_a_deg + VOICE_EXT_A_STEP_DEG;
            *p_ext_a_deg = (next_a > VOICE_EXT_A_TARGET_DEG) ? VOICE_EXT_A_TARGET_DEG : next_a;
            servo_set_angleA(*p_ext_a_deg);
        }

        if ((*p_ext_d_deg > VOICE_EXT_D_TARGET_DEG_MIN) && (*p_ext_d_deg < 90U))
        {
            uint32_t next_d = (*p_ext_d_deg > VOICE_EXT_D_STEP_DEG) ? (*p_ext_d_deg - VOICE_EXT_D_STEP_DEG) : 0U;
            *p_ext_d_deg = (next_d < VOICE_EXT_D_TARGET_DEG_MIN) ? VOICE_EXT_D_TARGET_DEG_MIN : next_d;
            servo_set_angleD(*p_ext_d_deg);
        }
        if ((*p_ext_d_deg < VOICE_EXT_D_TARGET_DEG_MAX) && (*p_ext_d_deg > 90U))
        {
            uint32_t next_d = *p_ext_d_deg + VOICE_EXT_D_STEP_DEG;
            *p_ext_d_deg = (next_d > VOICE_EXT_D_TARGET_DEG_MAX) ? VOICE_EXT_D_TARGET_DEG_MAX : next_d;
            servo_set_angleD(*p_ext_d_deg);
        }
    }
}

static uint8_t mpu_update_and_apply(int32_t ax0,
                                   int32_t ay0,
                                   mpu_att_state_t * p_att,
                                   uint32_t * p_rot_deg,
                                   uint32_t * p_ext_a_deg,
                                   uint32_t * p_ext_c_deg,
                                   uint32_t * p_ext_d_deg,
                                   uint32_t * p_dbg_tick,
                                   uint8_t recording,
                                   action_frame_t * frames,
                                   uint32_t max_frames,
                                   uint32_t * p_frame_count)
{
    mpu6050_raw_t raw;
    fsp_err_t err = MPU6050_ReadRaw(&raw);
    if (FSP_SUCCESS != err)
    {
        return 0U;
    }
    //误差值=测得值-平均值
    int32_t ax = (int32_t) raw.ax - ax0;
    int32_t ay = (int32_t) raw.ay - ay0;
    int32_t az = (int32_t) raw.az;
    if (iabs32(ax) < AX_DEADZONE_COUNTS) { ax = 0; }//如果晃荡幅度<2500则判断是垃圾数据
    if (iabs32(ay) < AY_DEADZONE_COUNTS) { ay = 0; }

    float roll_deg = 0.0f;
    float pitch_deg = 0.0f;
    mpu_att_update(p_att,
                   ax,
                   ay,
                   az,
                   (int32_t) raw.gx,
                   (int32_t) raw.gy,
                   ((float) MPU_CTRL_PERIOD_MS) / 1000.0f,
                   &roll_deg,
                   &pitch_deg);

    int32_t rot_target_i32 = (int32_t) ROT_CENTER_DEG + (int32_t) roll_deg;
    int32_t ext_delta_deg = (int32_t) pitch_deg;
    //微小抖动过滤 的量化处理 ，核心是把小于 2° 的微小变化直接过滤掉
    ext_delta_deg = (ext_delta_deg / (int32_t) EXT_DEG_QUANT) * (int32_t) EXT_DEG_QUANT;
    //x轴的目标角度
    int32_t ext_common_target_i32 = (int32_t) EXT_COMMON_CENTER_DEG + ext_delta_deg;

    uint32_t rot_target = clamp_i32_to_u32(rot_target_i32, (uint32_t) ROT_MIN_DEG, (uint32_t) ROT_MAX_DEG);
    //偏移角度
    int32_t ext_delta = ext_common_target_i32 - (int32_t) EXT_COMMON_CENTER_DEG;

    int32_t ext_a_target_i32 = (int32_t) EXT_A_CENTER_DEG + (EXT_A_DIR * ext_delta * EXT_A_GAIN_NUM / EXT_A_GAIN_DEN);
    int32_t ext_c_target_i32 = (int32_t) EXT_C_CENTER_DEG + (EXT_C_DIR * ext_delta * EXT_C_GAIN_NUM / EXT_C_GAIN_DEN);
    int32_t ext_d_target_i32 = (int32_t) EXT_D_CENTER_DEG + (EXT_D_DIR * ext_delta * EXT_D_GAIN_NUM / EXT_D_GAIN_DEN);

    uint32_t ext_a_target = clamp_i32_to_u32(ext_a_target_i32, (uint32_t) EXT_A_MIN_DEG, (uint32_t) EXT_A_MAX_DEG);
    uint32_t ext_c_target = clamp_i32_to_u32(ext_c_target_i32, (uint32_t) EXT_C_MIN_DEG, (uint32_t) EXT_C_MAX_DEG);
    uint32_t ext_d_target = clamp_i32_to_u32(ext_d_target_i32, (uint32_t) EXT_D_MIN_DEG, (uint32_t) EXT_D_MAX_DEG);

    uint32_t rot_next = step_towards_u32(*p_rot_deg, rot_target, ROT_STEP_DEG);
    uint32_t ext_a_next = step_towards_u32(*p_ext_a_deg, ext_a_target, EXT_A_STEP_DEG);
    uint32_t ext_c_next = step_towards_u32(*p_ext_c_deg, ext_c_target, EXT_C_STEP_DEG);
    uint32_t ext_d_next = step_towards_u32(*p_ext_d_deg, ext_d_target, EXT_D_STEP_DEG);

    if (rot_next != *p_rot_deg)
    {
        *p_rot_deg = rot_next;
        servo_set_angleE(*p_rot_deg);
    }
    if (ext_a_next != *p_ext_a_deg)
    {
        *p_ext_a_deg = ext_a_next;
        servo_set_angleA(*p_ext_a_deg);
    }
    if (ext_c_next != *p_ext_c_deg)
    {
        *p_ext_c_deg = ext_c_next;
        servo_set_angleC(*p_ext_c_deg);
    }
    if (ext_d_next != *p_ext_d_deg)
    {
        *p_ext_d_deg = ext_d_next;
        servo_set_angleD(*p_ext_d_deg);
    }

    record_frame_if_enabled(recording,
                            frames,
                            max_frames,
                            p_frame_count,
                            *p_ext_a_deg,
                            *p_ext_c_deg,
                            *p_ext_d_deg,
                            *p_rot_deg,
                            (uint16_t) MPU_CTRL_PERIOD_MS);

    *p_dbg_tick += MPU_CTRL_PERIOD_MS;
    if (*p_dbg_tick >= MPU_DEBUG_PERIOD_MS)
    {
        *p_dbg_tick = 0U;
        printf("mpu ax=%d ay=%d az=%d gx=%d gy=%d | roll=%.1f pitch=%.1f | rotE=%lu | extA=%lu extC=%lu extD=%lu\r\n",
               raw.ax, raw.ay, raw.az, raw.gx, raw.gy,
               (double) roll_deg, (double) pitch_deg,
               (unsigned long) (*p_rot_deg),
               (unsigned long) (*p_ext_a_deg), (unsigned long) (*p_ext_c_deg), (unsigned long) (*p_ext_d_deg));
    }

    return 1U;
}


/*******************************************************************************************************************//**
 * main() is generated by the RA Configuration editor and is used to generate threads if an RTOS is used.  This function
 * is called by main() when no RTOS is used.
 **********************************************************************************************************************/
void hal_entry(void)
{
    /* TODO: add your own code here */
    //串口初始化
    drv_uart_init();
    printf("uart_Init success!\r\n");

    wifi_tcp_server_t server = {0};//将所有结构成员变量置为0
    int wifi_ok = 0;

    printf("\r\n==== WIFI BEGIN ====\r\n");
    int wret = Wifi_Init();
    printf("Wifi_Init ret=%d\r\n", wret);

//***************环环相扣*********//
    if (0 == wret)
    {
        wret = Wifi_Reset();
        printf("Wifi_Reset ret=%d\r\n", wret);
    }

    if (0 == wret)
    {
        printf("Wifi_ConnectSTA ssid=www\r\n");
        wret = Wifi_ConnectSTA("www", "www123456");
        printf("Wifi_ConnectSTA ret=%d\r\n", wret);
    }

    if (0 == wret)
    {
        (void) Wifi_QueryIpSta();
    }

    if (0 == wret)
    {
        wret = Wifi_TcpServerStart(8000U, &server);
        printf("Wifi_TcpServerStart ret=%d, server_socket=%lu\r\n", wret, (unsigned long) server.server_socket);
    }
    if (0 == wret)
    {
        wifi_ok = 1;
    }
    printf("==== WIFI END ====\r\n\r\n");
//*************直到wifi配置结束***********//


    //舵机初始化
    servo_init();
    //MPU6050初始化
    fsp_err_t err = MPU6050_Init();
    printf("MPU6050 init err=%d\r\n", (int) err);
    uint8_t who = 0U;
    err = MPU6050_WhoAmI(&who);
    printf("MPU6050 WHO_AM_I err=%d, who=0x%02X\r\n", (int) err, who);

    int32_t ax0 = 0;
    int32_t ay0 = 0;
    uint32_t cal_cnt = 0U;//计数器 ：记录成功捕获的数据

    for (uint32_t i = 0; i < 50U; i++)//计算均值减少误差
    {
        mpu6050_raw_t raw;
        err = MPU6050_ReadRaw(&raw);
        if (FSP_SUCCESS == err)
        {
            //accumulation
            ax0 += raw.ax;//水平旋转是y
            ay0 += raw.ay;//垂直运动是x
            //just record the number of successful reads
            cal_cnt++;
        }
        R_BSP_SoftwareDelay(10U, BSP_DELAY_UNITS_MILLISECONDS);
    }
    if (cal_cnt > 0U)//求取平均值 提高精确
    {
        //Only record the correct values and take the average
        ax0 /= (int32_t) cal_cnt;
        ay0 /= (int32_t) cal_cnt;
    }
    printf("MPU calib ax0=%ld ay0=%ld cnt=%lu\r\n", (long) ax0, (long) ay0, (unsigned long) cal_cnt);
    //再次初始化ACD以及E舵机的角度
    uint32_t rot_deg = (uint32_t) ROT_CENTER_DEG;
    uint32_t ext_a_deg = (uint32_t) EXT_A_CENTER_DEG;
    uint32_t ext_c_deg = (uint32_t) EXT_C_CENTER_DEG;
    uint32_t ext_d_deg = (uint32_t) EXT_D_CENTER_DEG;
    mpu_att_state_t att;
    mpu_att_init(&att);
    uint32_t dbg_tick = 0U;//调试打印的计数器

    action_frame_t action_frames[ACTION_MAX_FRAMES];//存储帧的数组
    uint32_t action_frame_count = 0U;//当前存放帧数量
    uint8_t  action_recording = 0U;//是否在记录的标志
    uint8_t  action_playing = 0U;//是否在复现的开关
    uint32_t action_play_idx = 0U;//记录当前复现到第几帧了

    uint32_t wifi_poll_tick = 0U;//WiFi 轮询计时器，用于定期检查客户端连接
    uint8_t wifi_servo_smooth_init = 0U;
    uint32_t wifi_tgt_a = 90U;
    uint32_t wifi_tgt_b = 90U;
    uint32_t wifi_tgt_c = 90U;
    uint32_t wifi_tgt_d = 90U;
    uint32_t wifi_tgt_e = 90U;
    uint32_t wifi_cur_a = 90U;
    uint32_t wifi_cur_b = 90U;
    uint32_t wifi_cur_c = 90U;
    uint32_t wifi_cur_d = 90U;
    uint32_t wifi_cur_e = 90U;

    static char http_acc[512];
    static uint32_t http_acc_len = 0U;

    while(1){

        uart_recv_asr_cmd(); // 检测语音模块的串口指令

        if (voice_wireless_control_is_set())
        {
            if (CONTROL_MODE_WIRELESS != g_control_mode)
            {
                g_control_mode = CONTROL_MODE_WIRELESS;
                printf("CTRL: WIRELESS(voice)\r\n");
            }
        }
        else
        {
            if (CONTROL_MODE_WIRED != g_control_mode)
            {
                g_control_mode = CONTROL_MODE_WIRED;
                wifi_servo_smooth_init = 0U;
                printf("CTRL: WIRED(voice)\r\n");
            }
        }

        if (CONTROL_MODE_WIRELESS == g_control_mode)
        {
            if (!wifi_servo_smooth_init)
            {
                wifi_cur_a = servo_get_angleA();
                wifi_cur_b = servo_get_angleB();
                wifi_cur_c = servo_get_angleC();
                wifi_cur_d = servo_get_angleD();
                wifi_cur_e = servo_get_angleE();
                wifi_tgt_a = wifi_cur_a;
                wifi_tgt_b = wifi_cur_b;
                wifi_tgt_c = wifi_cur_c;
                wifi_tgt_d = wifi_cur_d;
                wifi_tgt_e = wifi_cur_e;
                wifi_servo_smooth_init = 1U;
            }

            wifi_poll_tick += MPU_CTRL_PERIOD_MS;

            if (wifi_ok)
            {
                if (0U == server.client_socket)
                {
                    if (wifi_poll_tick >= 200U)
                    {
                        wifi_poll_tick = 0U;
                        if (0 == Wifi_TcpServerPollClient(&server))
                        {
                            printf("WiFi: Client connected, socket=%lu\r\n", (unsigned long) server.client_socket);
                            http_acc_len = 0U;
                        }
                    }
                }
                else
                {
                    uint8_t rx[256];
                    int rn = Wifi_TcpRecv(server.client_socket, rx, sizeof(rx), 200U);
                    if (rn < 0)
                    {
                        printf("WiFi: Client disconnected, err=%d\r\n", rn);
                        server.client_socket = 0U;
                        http_acc_len = 0U;
                        continue;
                    }
                    if (rn > 0)
                    {
                        if (rn >= (int) sizeof(rx))
                        {
                            rn = (int) (sizeof(rx) - 1U);
                        }
                        rx[rn] = 0U;

                        if (http_acc_len >= (uint32_t) (sizeof(http_acc) - 1U))
                        {
                            http_acc_len = 0U;
                        }

                        uint32_t copy = (uint32_t) rn;
                        if ((http_acc_len + copy) >= (uint32_t) (sizeof(http_acc) - 1U))
                        {
                            copy = (uint32_t) (sizeof(http_acc) - 1U) - http_acc_len;
                        }
                        if (copy > 0U)
                        {
                            memcpy(&http_acc[http_acc_len], rx, copy);
                            http_acc_len += copy;
                            http_acc[http_acc_len] = '\0';
                        }

                        char const * req_end = strstr(http_acc, "\r\n");
                        if (NULL == req_end)
                        {
                            req_end = strstr(http_acc, " HTTP/");
                        }
                        if (req_end != NULL)
                        {
                            if (0 == strncmp(http_acc, "GET ", 4))
                            {
                                char const * path = &http_acc[4];
                                char const * sp = strchr(path, ' ');
                                if (NULL != sp)
                                {
                                    char tmp_path[160];
                                    size_t len = (size_t) (sp - path);
                                    if (len >= sizeof(tmp_path))
                                    {
                                        len = sizeof(tmp_path) - 1U;
                                    }
                                    memcpy(tmp_path, path, len);
                                    tmp_path[len] = '\0';

                                    printf("HTTP: %s\r\n", tmp_path);

                                    if (0 == strcmp(tmp_path, "/"))
                                    {
                                        wifi_http_send_html_page(server.client_socket);
                                    }
                                    else if (0 == strncmp(tmp_path, "/set", 4))
                                    {
                                        uint32_t v;
                                        if (http_try_parse_set_query(tmp_path, 'A', &v)) { wifi_tgt_a = v; }
                                        if (http_try_parse_set_query(tmp_path, 'B', &v)) { wifi_tgt_b = v; }
                                        if (http_try_parse_set_query(tmp_path, 'C', &v)) { wifi_tgt_c = v; }
                                        if (http_try_parse_set_query(tmp_path, 'D', &v)) { wifi_tgt_d = v; }
                                        if (http_try_parse_set_query(tmp_path, 'E', &v)) { wifi_tgt_e = v; }

                                        wifi_http_send(server.client_socket,
                                                      "HTTP/1.1 200 OK\r\n"
                                                      "Cache-Control: no-store\r\n"
                                                      "Content-Length: 0\r\n"
                                                      "Connection: keep-alive\r\n"
                                                      "\r\n");
                                    }
                                    else if (0 == strcmp(tmp_path, "/favicon.ico"))
                                    {
                                        wifi_http_send(server.client_socket,
                                                      "HTTP/1.1 404 Not Found\r\n"
                                                      "Content-Length: 0\r\n"
                                                      "Connection: keep-alive\r\n"
                                                      "\r\n");
                                    }
                                    else
                                    {
                                        wifi_http_send(server.client_socket,
                                                      "HTTP/1.1 404 Not Found\r\n"
                                                      "Content-Length: 0\r\n"
                                                      "Connection: keep-alive\r\n"
                                                      "\r\n");
                                    }
                                    http_acc_len = 0U;
                                    continue;
                                }
                            }

                            http_acc_len = 0U;
                            continue;
                        }

                        if ((http_acc_len >= 4U) && (0 == strncmp(http_acc, "GET ", 4)))
                        {
                            continue;
                        }
                        char servo = 0;
                        uint32_t deg = 0U;
                        if (wifi_try_parse_servo_cmd((char const *) rx, &servo, &deg))
                        {
                            if ('A' == servo)
                            {
                                wifi_tgt_a = deg;
                            }
                            else if ('B' == servo)
                            {
                                wifi_tgt_b = deg;
                            }
                            else if ('C' == servo)
                            {
                                wifi_tgt_c = deg;
                            }
                            else if ('D' == servo)
                            {
                                wifi_tgt_d = deg;
                            }
                            else
                            {
                                wifi_tgt_e = deg;
                            }

                            printf("WiFi: %c=%lu\r\n", servo, (unsigned long) deg);
                        }
                        else
                        {
                            printf("WiFi: RX=%s\r\n", (char const *) rx);
                        }
                    }
                    else if (rn < 0)
                    {
                        server.client_socket = 0U;
                        http_acc_len = 0U;
                        printf("WiFi: Client disconnected\r\n");
                    }
                    else
                    {
                    }
                }
            }

            wifi_cur_a = step_towards_u32(wifi_cur_a, wifi_tgt_a, 4U);
            wifi_cur_b = step_towards_u32(wifi_cur_b, wifi_tgt_b, 4U);
            wifi_cur_c = step_towards_u32(wifi_cur_c, wifi_tgt_c, 4U);
            wifi_cur_d = step_towards_u32(wifi_cur_d, wifi_tgt_d, 2U);
            wifi_cur_e = step_towards_u32(wifi_cur_e, wifi_tgt_e, 2U);

            servo_set_angleA(wifi_cur_a);
            servo_set_angleB(wifi_cur_b);
            servo_set_angleC(wifi_cur_c);
            servo_set_angleD(wifi_cur_d);
            servo_set_angleE(wifi_cur_e);

            ext_a_deg = wifi_cur_a;
            ext_c_deg = wifi_cur_c;
            ext_d_deg = wifi_cur_d;

            R_BSP_SoftwareDelay(MPU_CTRL_PERIOD_MS, BSP_DELAY_UNITS_MILLISECONDS);
            continue;
        }

         handle_voice_mem_events(action_frames,
                                 ACTION_MAX_FRAMES,
                                 &action_frame_count,//已经录制帧数
                                 &action_recording,//记录标志
                                 &action_playing,//重现标志
                                 &action_play_idx);//复现索引

         if (servo_emergency_stop_is_set())
         {
             action_playing = 0U;
             action_play_idx = 0U;

             R_BSP_SoftwareDelay(MPU_CTRL_PERIOD_MS, BSP_DELAY_UNITS_MILLISECONDS);
             continue;
         }

         if (action_playing)
         {
             if (action_frame_count > 0U)
             {
                 if (playback_step(action_frames, action_frame_count, &action_play_idx))
                 {
                     continue;
                 }
                 action_play_idx = 0U;
                 continue;
             }
             action_playing = 0U;
             action_play_idx = 0U;
         }

         uint8_t voice_extend_mode = voice_extend_mode_is_set();
         if (voice_extend_mode)
         {
             voice_extend_handle_steps(&ext_a_deg, &ext_d_deg);
             R_BSP_SoftwareDelay(MPU_CTRL_PERIOD_MS, BSP_DELAY_UNITS_MILLISECONDS);
             continue;
         }

         //开始逐帧录制
         (void) mpu_update_and_apply(ax0,
                                    ay0,
                                    &att,
                                    &rot_deg,
                                    &ext_a_deg,
                                    &ext_c_deg,
                                    &ext_d_deg,
                                    &dbg_tick,
                                    action_recording,
                                    action_frames,
                                    ACTION_MAX_FRAMES,
                                    &action_frame_count);

         R_BSP_SoftwareDelay(MPU_CTRL_PERIOD_MS, BSP_DELAY_UNITS_MILLISECONDS);
       }

#if BSP_TZ_SECURE_BUILD
    /* Enter non-secure code */
    R_BSP_NonSecureEnter();
#endif
}

/*******************************************************************************************************************//**
 * This function is called at various points during the startup process.  This implementation uses the event that is
 * called right before main() to set up the pins.
 *
 * @param[in]  event    Where at in the start up process the code is currently at
 **********************************************************************************************************************/
void R_BSP_WarmStart(bsp_warm_start_event_t event)
{
    if (BSP_WARM_START_RESET == event)
    {
#if BSP_FEATURE_FLASH_LP_VERSION != 0

        /* Enable reading from data flash. */
        R_FACI_LP->DFLCTL = 1U;

        /* Would normally have to wait tDSTOP(6us) for data flash recovery. Placing the enable here, before clock and
         * C runtime initialization, should negate the need for a delay since the initialization will typically take more than 6us. */
#endif
    }

    if (BSP_WARM_START_POST_C == event)
    {
        /* C runtime environment and system clocks are setup. */

        /* Configure pins. */
        R_IOPORT_Open (&g_ioport_ctrl, g_ioport.p_cfg);
    }
}

#if BSP_TZ_SECURE_BUILD

FSP_CPP_HEADER
BSP_CMSE_NONSECURE_ENTRY void template_nonsecure_callable ();

/* Trustzone Secure Projects require at least one nonsecure callable function in order to build (Remove this if it is not required to build). */
BSP_CMSE_NONSECURE_ENTRY void template_nonsecure_callable ()
{

}
FSP_CPP_FOOTER

#endif
