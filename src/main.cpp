extern "C" {
#include <psp2/appmgr.h>
#include <psp2/apputil.h>
#include <psp2/io/fcntl.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/libssl.h>
#include <psp2/sysmodule.h>

#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <curl/curl.h>

#include "appconfig.h"
#include "display.h"
#include "driveapi.h"
#include "httpnet.h"
#include "logger.h"
#include "savedata.h"
#include "savefile.h"

#include "gdrive.hpp"
}

#include <imgui_vita2d/imgui_vita.h>
#include <vita2d.h>

#include <vector>

#include "fs.hpp"
#include "gui.hpp"

#define wait3s sceKernelDelayThread(3 * 1000000)

int main(int argc, char *argv[]) {
  // logger_init();
  // printf("\e[0;32m");
  // printf("PS Vita Sync v%s\n", APP_VERSION);
  // printf("\e[0;37m");

  // // displaySavedataFolder("ux0:/user/00/savedata/");

  // // displayMainMenu();

  // // Inits
  // OpenSSL_add_all_algorithms();
  // httpNetInit();
  // printf("Inits done\n");

  // HttpResponse_t response = requestDeviceAndUserCodes();
  // handleTheAuthorizationServerResponse(response.buffer);
  // freeHttpResponse(response);

  // // char *access_token = getOAuth2Token();
  // // printf("access_token granted\n");

  // // char *mhf2ndgId = "18YLl-vKzWdKYC-2FGSNN949Ddt7-i5C0";
  // // char *sfo_file = "18NKESueoRL1hszPCS1-Frrwr9x8qEpKO";
  // // HttpResponse_t json =
  // //     getFile(mhf2ndgId, "?fields=modifiedTime", false, access_token);
  // // printf("mhf2ndgId modifiedTime find\n");

  // // unsigned long long int lmt_driveFile = getModifiedTime(json.buffer);
  // // freeHttpResponse(json);

  // // long long lmt_localFile =
  // //     getLastModificationTime("ux0:/pspemu/PSP/SAVEDATA/ULUS10391/");

  // // printf("cloud: %llu\n", lmt_driveFile);
  // // printf("local: %lld\n", lmt_localFile);

  // // // Upload the local version or download the cloud version
  // // if (lmt_driveFile >= lmt_localFile) {
  // //   int res = downloadSavefile(mhf2ndgId,
  // // "ux0:/pspemu/PSP/SAVEDATA/ULUS10391/MHP2NDG.BIN",
  // //                              access_token);
  // //   if (res == 1) {
  // //     printf("MHP2NDG savefile correctly downloaded\n");
  // //     res = downloadSavefile(sfo_file,
  // // "ux0:/pspemu/PSP/SAVEDATA/ULUS10391/PARAM.SFO",
  // //                            access_token);
  // //     if (res == 1) {
  // //       printf("param sfo savefile correctly downloaded\n");
  // //     } else {
  // //       printf("error on download savefile %x\n", res);
  // //     }
  // //   } else {
  // //     printf("error on download savefile %x\n", res);
  // //   }
  // // } else {
  // //   httpNetClose();
  // //   curl_global_init(CURL_GLOBAL_ALL);
  // //   int res =
  // //   uploadSavefile("ux0:/pspemu/PSP/SAVEDATA/ULUS10391/MHP2NDG.BIN",
  // //                            mhf2ndgId, access_token);
  // //   if (res == 1) {
  // //     printf("MHP2NDG savefile correctly uploaded\n");
  // //   } else {
  // //     printf("error on upload savefile %d\n", res);
  // //     printf("curl_easy_perform() failed: %s\n",
  // //     curl_easy_strerror(res));
  // //   }
  // // }

  // httpNetClose();
  // // free(access_token);

  // printf("\nSync correctly done, app will close in 3s\n");
  // wait3s;
  // logger_exit(0);

  // RmlUI

  // vita2d_init();

  // // Setup ImGui binding
  // ImGui::CreateContext();
  // ImGuiIO &io = ImGui::GetIO();
  // (void)io;
  // ImGui_ImplVita2D_Init();

  // // Setup style
  // ImGui::StyleColorsDark();

  // bool show_demo_window = true;
  // bool show_another_window = false;
  // ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

  // ImGui_ImplVita2D_TouchUsage(true);
  // ImGui_ImplVita2D_UseIndirectFrontTouch(false);
  // ImGui_ImplVita2D_UseRearTouch(true);
  // ImGui_ImplVita2D_GamepadUsage(true);

  // // Main loop
  // bool done = false;
  // while (!done) {
  //   vita2d_start_drawing();
  //   vita2d_clear_screen();

  //   ImGui_ImplVita2D_NewFrame();

  //   if (ImGui::BeginMainMenuBar()) {
  //     if (ImGui::BeginMenu("Debug")) {
  //       ImGui::EndMenu();
  //     }
  //     ImGui::EndMainMenuBar();
  //   }

  //   // 1. Show a simple window.
  //   // Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets
  //   // automatically appears in a window called "Debug".
  //   {
  //     static float f = 0.0f;
  //     static int counter = 0;
  //     ImGui::Text("Hello, world!"); // Display some text (you can use a
  //     format
  //                                   // string too)
  //     ImGui::SliderFloat("float", &f, 0.0f,
  //                        1.0f); // Edit 1 float using a slider from 0.0f
  //                        to 1.0f
  //     ImGui::ColorEdit3(
  //         "clear color",
  //         (float *)&clear_color); // Edit 3 floats representing a color

  //     ImGui::Checkbox(
  //         "Demo Window",
  //         &show_demo_window); // Edit bools storing our windows open/close
  //         state
  //     ImGui::Checkbox("Another Window", &show_another_window);

  //     if (ImGui::Button("Button")) // Buttons return true when clicked (NB:
  //     most
  //                                  // widgets return true when
  //                                  edited/activated)
  //       counter++;
  //     ImGui::SameLine();
  //     ImGui::Text("counter = %d", counter);

  //     ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
  //                 1000.0f / ImGui::GetIO().Framerate,
  //                 ImGui::GetIO().Framerate);
  //   }

  //   // 2. Show another simple window. In most cases you will use an explicit
  //   // Begin/End pair to name your windows.
  //   if (show_another_window) {
  //     ImGui::Begin("Another Window", &show_another_window);
  //     ImGui::Text("Hello from another window!");
  //     if (ImGui::Button("Close Me"))
  //       show_another_window = false;
  //     ImGui::End();
  //   }

  //   // 3. Show the ImGui demo window. Most of the sample code is in
  //   // ImGui::ShowDemoWindow(). Read its code to learn more about Dear ImGui!

  //   ImGui::Render();
  //   ImGui_ImplVita2D_RenderDrawData(ImGui::GetDrawData());

  //   vita2d_end_drawing();
  //   vita2d_swap_buffers();
  //   sceDisplayWaitVblankStart();
  // }

  // // Cleanup
  // ImGui_ImplVita2D_Shutdown();
  // ImGui::DestroyContext();

  // vita2d_fini();

  // SDL2
  rectangle();

  // printf("\nSync correctly done, app will close in 3s\n");
  // wait3s;
  // logger_exit(0);

  return 0;
}
