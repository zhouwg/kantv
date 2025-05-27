package kantvai.tool.blankj.utilcode.constant;

import android.Manifest.permission;
import android.annotation.SuppressLint;
import android.os.Build;
import androidx.annotation.StringDef;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;


/**
 * <pre>
 *     author: Blankj
 *     blog  : http://blankj.com
 *     time  : 2017/12/29
 *     desc  : constants of permission
 * </pre>
 */
@SuppressLint("InlinedApi")
public final class PermissionConstants {


    public static final String CAMERA               = "CAMERA";
    public static final String MICROPHONE           = "MICROPHONE";
    public static final String PHONE                = "PHONE";
    public static final String SENSORS              = "SENSORS";
    public static final String STORAGE              = "STORAGE";
    public static final String ACTIVITY_RECOGNITION = "ACTIVITY_RECOGNITION";

    private static final String[] GROUP_CALENDAR             = {
            permission.READ_CALENDAR, permission.WRITE_CALENDAR
    };
    private static final String[] GROUP_CAMERA               = {
            permission.CAMERA
    };
    private static final String[] GROUP_MICROPHONE           = {
            permission.RECORD_AUDIO
    };
    private static final String[] GROUP_PHONE                = {
            permission.READ_PHONE_STATE, permission.READ_PHONE_NUMBERS, permission.CALL_PHONE,
            permission.READ_CALL_LOG, permission.WRITE_CALL_LOG, permission.ADD_VOICEMAIL,
            permission.USE_SIP, permission.PROCESS_OUTGOING_CALLS, permission.ANSWER_PHONE_CALLS
    };
    private static final String[] GROUP_PHONE_BELOW_O        = {
            permission.READ_PHONE_STATE, permission.READ_PHONE_NUMBERS, permission.CALL_PHONE,
            permission.READ_CALL_LOG, permission.WRITE_CALL_LOG, permission.ADD_VOICEMAIL,
            permission.USE_SIP, permission.PROCESS_OUTGOING_CALLS
    };
    private static final String[] GROUP_SENSORS              = {
            permission.BODY_SENSORS
    };
    private static final String[] GROUP_STORAGE              = {
            permission.READ_EXTERNAL_STORAGE, permission.WRITE_EXTERNAL_STORAGE,
    };
    private static final String[] GROUP_ACTIVITY_RECOGNITION = {
            permission.ACTIVITY_RECOGNITION,
    };

    @StringDef({CAMERA, MICROPHONE, PHONE, SENSORS, STORAGE,})
    @Retention(RetentionPolicy.SOURCE)
    public @interface PermissionGroup {
    }

    public static String[] getPermissions(@PermissionGroup final String permission) {
        if (permission == null) return new String[0];
        switch (permission) {
            case CAMERA:
                return GROUP_CAMERA;
            case MICROPHONE:
                return GROUP_MICROPHONE;
            case PHONE:
                if (Build.VERSION.SDK_INT < Build.VERSION_CODES.O) {
                    return GROUP_PHONE_BELOW_O;
                } else {
                    return GROUP_PHONE;
                }
            case SENSORS:
                return GROUP_SENSORS;
            case STORAGE:
                return GROUP_STORAGE;
            case ACTIVITY_RECOGNITION:
                return GROUP_ACTIVITY_RECOGNITION;
        }
        return new String[]{permission};
    }
}
