#include "dialog.h"

int dialogMsg(char *msg) {

  SceMsgDialogUserMessageParam usrMsgParam = {.buttonType =
                                                  SCE_MSG_DIALOG_BUTTON_TYPE_OK,
                                              .msg = msg,
                                              .buttonParam = NULL};

  SceMsgDialogParam param = {.sdkVersion = NULL,
                             .mode = SCE_MSG_DIALOG_MODE_USER_MSG,
                             .userMsgParam = &usrMsgParam};

  return sceMsgDialogInit(&param);
}
