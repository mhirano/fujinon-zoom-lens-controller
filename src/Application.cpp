//
// Created by Masahiro Hirano <masahiro.dll@gmail.com>
//

#include "Application.h"
#include <opencv2/opencv.hpp>
#include "imgui_apps.h"
#include "implot.h"

#include "AppMsg.h"
#include "Engine.h"
#include "ImageTexture.h"
#include "Config.h"
#include "Logger.h"
#include "Utility.h"

#include "FujinonZoomLensCom.h"

Application::Application() {
// Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
//        return -1;
    }

// Decide GL+GLSL versions
#if __APPLE__
// GL 3.2 Core + GLSL 150
//    const char* glsl_version = "#version 150";
    glsl_version = "#version 150";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
    // GL 3.0 + GLSL 130
    glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

// Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
#if defined(DEBUG) || defined(_DEBUG)
    window = SDL_CreateWindow("Fujinon zoom lens controller - debug", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 720, 540, window_flags);
#else
    window = SDL_CreateWindow("Fujinon zoom lens controller", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 720, 540, window_flags);
#endif
    gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

// Initialize OpenGL loader
#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
    bool err = gl3wInit() != 0;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
    bool err = glewInit() != GLEW_OK;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
    bool err = gladLoadGL() == 0;
#else
    bool err = false; // If you use IMGUI_IMPL_OPENGL_LOADER_CUSTOM, your loader is likely to requires some form of initialization.
#endif
    if (err)
    {
        fprintf(stderr, "Failed to initialize OpenGL loader!\n");
//        return 1;
    }
}

Application::~Application(){
// Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

bool Application::run(){

// Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); // (void)io;
//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

// Setup Dear ImGui style
    ImGui::StyleColorsDark();
//ImGui::StyleColorsClassic();
    ImVec4 clear_color = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);    // Set background color

// Setup Platform/Renderer bindings
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version.c_str());

// Load Fonts
// - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
// - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
// - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
// - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
// - Read 'misc/fonts/README.txt' for more instructions and details.
// - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
//io.Fonts->AddFontDefault();
//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
//io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
//io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
//ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
//IM_ASSERT(font != NULL);
#ifdef _MSC_VER
	ImFont* font = io.Fonts->AddFontFromFileTTF("../../font/mplus-1p-medium.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    IM_ASSERT(font != NULL);
#else
	ImFont* font = io.Fonts->AddFontFromFileTTF("../font/mplus-1p-medium.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
	IM_ASSERT(font != NULL);
#endif

// Window capture and recording
    bool requestedWindowCapture = false;
    enum WINDOW_RECORDING_STATUS {PAUSED = 0, REQUESTED = 1, RECORDING = 2};
    WINDOW_RECORDING_STATUS windowRecordingStatus = WINDOW_RECORDING_STATUS::PAUSED;
    cv::VideoWriter writer;
    std::string windowRecordingFileName;

// Initialize application config
    Config::get_instance();

// Setup logger
    ExampleAppLog my_log;
    Logger::get_instance().setExportDirectory(Config::get_instance().resultDirectory());

    AppMsgPtr appMsg = std::make_shared<AppMsg>();
    std::shared_ptr<EngineOffline> engine(new EngineOffline(appMsg));
    std::map<std::string, ImageTexture> texturePool;

    enum SHOW_IMAGE_MODE {IMGUI = 0, OPENCV = 1};
    static int selectedShowImageMode = SHOW_IMAGE_MODE::IMGUI;

// Main loop
    bool done = false;
    while (!done)
    {
// Poll and handle events (inputs, window resize, etc.)
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }


// Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame(window);
        ImGui::NewFrame();

// Dear ImGui demo
        {
            ImGui::ShowDemoWindow();
        }

// Commands
        {
            ImGui::Begin("Commands");
            /// Check if the worker finished

            static WORKER_STATUS observedWorkerStatus(WORKER_STATUS::IDLE);
            if (observedWorkerStatus == WORKER_STATUS::RUNNING &&
                engine->getWorkerStatus() == WORKER_STATUS::IDLE) {
                engine->reset();
            }
            observedWorkerStatus = engine->getWorkerStatus();

            if (ImGui::Button("Run fujinon setZoomRatio lens")) {
                if(observedWorkerStatus == WORKER_STATUS::IDLE){
                    engine->run();
                } else {
                    ImGui::Text("Command ignored");
                }
            }
            ImGui::SameLine();
            if (observedWorkerStatus == WORKER_STATUS::IDLE){
                ImGui::Text("Worker: idle");
            } else if (observedWorkerStatus == WORKER_STATUS::RUNNING){
                ImGui::Text("Worker: running");
            } else {
                ImGui::Text("Worker: unknown");
            }

			{
				ImGui::Text("Control");
				
				auto client = std::make_shared<FujinonZoomLensClient>(appMsg);
				FujinonZoomLensController zlc(std::static_pointer_cast<FujinonZoomLensClientTemplate>(client));

				// zoom
				{
					static float zoom_ratio = 1.0f;
					bool isChanged = ImGui::SliderFloat("Zoom", &zoom_ratio, 1.0f, 32.0f, "ratio = %3.1f");
					if (isChanged) {
						zlc.setZoomRatio(zoom_ratio);
					}
				}
				
				// focus
				{
					static float focus_meter = 3.0f;
					bool isChanged = ImGui::SliderFloat("Focus", &focus_meter, 3.0f, 150.0f, "%3.1f [m]");
					if (isChanged) {
						zlc.setFocus(focus_meter);
					}
				}

				// F number
				{
					static ImGuiComboFlags flags_F = 0;
					const char* items_F[] = { "OPEN", "F4", "F5.6", "F8", "F11", "F16", "CLOSE" };
					static int item_current_idx_F = 0;                    // Here our selection data is an index.
					const char* combo_label_F = items_F[item_current_idx_F];  // Label to preview before opening the combo (technically it could be anything)
					if (ImGui::BeginCombo("F number", combo_label_F, flags_F))
					{
						for (int n = 0; n < IM_ARRAYSIZE(items_F); n++)
						{
							const bool is_selected = (item_current_idx_F == n);
							if (ImGui::Selectable(items_F[n], is_selected)) {
								item_current_idx_F = n;

								if (items_F[item_current_idx_F] == "OPEN") {
									zlc.setF(FujinonZoomLensControllerUtil::ZOOM_LENS_F::OPEN);
								}
								else if (items_F[item_current_idx_F] == "F4") {
									zlc.setF(FujinonZoomLensControllerUtil::ZOOM_LENS_F::F4);
								}
								else if (items_F[item_current_idx_F] == "F5.6") {
									zlc.setF(FujinonZoomLensControllerUtil::ZOOM_LENS_F::F5_6);
								}
								else if (items_F[item_current_idx_F] == "F8") {
									zlc.setF(FujinonZoomLensControllerUtil::ZOOM_LENS_F::F8);
								}
								else if (items_F[item_current_idx_F] == "F11") {
									zlc.setF(FujinonZoomLensControllerUtil::ZOOM_LENS_F::F11);
								}
								else if (items_F[item_current_idx_F] == "F16") {
									zlc.setF(FujinonZoomLensControllerUtil::ZOOM_LENS_F::F16);
								}
								else if (items_F[item_current_idx_F] == "CLOSE") {
									zlc.setF(FujinonZoomLensControllerUtil::ZOOM_LENS_F::CLOSE);
								}
							}

							// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
							if (is_selected)
								ImGui::SetItemDefaultFocus();
						}
						ImGui::EndCombo();
					}
				}

				// filter
				{
					static ImGuiComboFlags flags_filter = 0;
					const char* items_filter[] = { "FILTER_CLEAR", "VISIBLE_LIGHT_CUT_FILTER" };
					static int item_current_idx_filter = 0;                    // Here our selection data is an index.
					const char* combo_label_filter = items_filter[item_current_idx_filter];  // Label to preview before opening the combo (technically it could be anything)
					if (ImGui::BeginCombo("Filter", combo_label_filter, flags_filter))
					{
						for (int n = 0; n < IM_ARRAYSIZE(items_filter); n++)
						{
							const bool is_selected = (item_current_idx_filter == n);
							if (ImGui::Selectable(items_filter[n], is_selected)) {
								item_current_idx_filter = n;

								if (items_filter[item_current_idx_filter] == "FILTER_CLEAR") {
									zlc.setFilter(FujinonZoomLensControllerUtil::ZOOM_LENS_FILTER::FILTER_CLEAR);
								}
								else if (items_filter[item_current_idx_filter] == "VISIBLE_LIGHT_CUT_FILTER") {
									zlc.setFilter(FujinonZoomLensControllerUtil::ZOOM_LENS_FILTER::VISIBLE_LIGHT_CUT_FILTER);
								}
							}

							// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
							if (is_selected)
								ImGui::SetItemDefaultFocus();
						}
						ImGui::EndCombo();
					}
				}

				// iris
				{
					static ImGuiComboFlags flags_iris = 0;
					const char* items_iris[] = { "REMOTE", "AUTO" };
					static int item_current_idx_iris = 0;                    // Here our selection data is an index.
					const char* combo_label_iris = items_iris[item_current_idx_iris];  // Label to preview before opening the combo (technically it could be anything)
					if (ImGui::BeginCombo("Iris", combo_label_iris, flags_iris))
					{
						for (int n = 0; n < IM_ARRAYSIZE(items_iris); n++)
						{
							const bool is_selected = (item_current_idx_iris == n);
							if (ImGui::Selectable(items_iris[n], is_selected)) {
								item_current_idx_iris = n;

								if (items_iris[item_current_idx_iris] == "REMOTE") {
									zlc.setIrisMode(FujinonZoomLensControllerUtil::ZOOM_LENS_IRIS::REMOTE);
								}
								else if (items_iris[item_current_idx_iris] == "AUTO") {
									zlc.setIrisMode(FujinonZoomLensControllerUtil::ZOOM_LENS_IRIS::AUTO);
								}
							}

							// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
							if (is_selected)
								ImGui::SetItemDefaultFocus();
						}
						ImGui::EndCombo();
					}
				}

				// Name
				{
					if (ImGui::Button("Get Name (first half)")) {
						zlc.getNameFirst();
					}
				}
				{
					if (ImGui::Button("Get Name (second half)")) {
						zlc.getNameSecond();
					}
				}

				// Serial number
				{
					if (ImGui::Button("Get Serial Number")) {
						zlc.getSerialNumber();
					}
				}

				// Get zoom position
				{
					if (ImGui::Button("Get zoom position")) {
						zlc.getZoomPosition();
					}
				}

				// Get zoom position
				{
					if (ImGui::Button("Get focus position")) {
						zlc.getFocusPosition();
					}
				}

			}


			ImGui::NewLine();
            if (ImGui::Button("Exit")){
				done = true;
            }
            ImGui::End();
        }

        /// Destroy OpenCV windows if exists
        if(selectedShowImageMode == SHOW_IMAGE_MODE::IMGUI) { /// Use ImGui
            cv::destroyAllWindows();
        }

        static float imguiImageScale = 1.0f; /// image scale for imgui rendering
        DispMsg *md = appMsg->displayMessenger->receive();
        if (md != nullptr) { // texture pool updated
            if(selectedShowImageMode == SHOW_IMAGE_MODE::IMGUI){
                texturePool.clear();
                for (auto img_in_pool:md->pool) {
                    std::string winname = img_in_pool.first;
                    ImVec2 imgSize(img_in_pool.second.cols, img_in_pool.second.rows);
                    ImGui::Begin(winname.c_str());
                    texturePool[img_in_pool.first].setImage(&img_in_pool.second);
                    ImGui::Image(texturePool[img_in_pool.first].getOpenglTexture(),
                                 ImVec2(imgSize.x * imguiImageScale, imgSize.y * imguiImageScale),
                                 ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f)
                                 );
                    ImGui::End();
                }
            } else if (selectedShowImageMode == SHOW_IMAGE_MODE::OPENCV){
                for (auto& img_in_pool:md->pool) {
                    std::string winname = img_in_pool.first;
                    cv::namedWindow(winname,cv::WINDOW_NORMAL);
                    cv::imshow(winname, img_in_pool.second);
                }
                cv::waitKey(1);
            }
        } else { // texture pool not updated
            if(selectedShowImageMode == SHOW_IMAGE_MODE::IMGUI){
                for (auto &texture: texturePool) {
                    std::string winname = texture.first;
                    ImVec2 imgSize = texturePool[texture.first].getSize();
                    ImGui::Begin(winname.c_str());
                    ImGui::Image(texturePool[texture.first].getOpenglTexture(),
                                 ImVec2(imgSize.x * imguiImageScale, imgSize.y * imguiImageScale),
                                 ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f)
                                 );
                    ImGui::End();
                }
            } else if (selectedShowImageMode == SHOW_IMAGE_MODE::OPENCV){
                cv::waitKey(1);
            }
        }

        //{
        //    static float xs1[1001], ys1[1001];
        //    double DEMO_TIME = ImGui::GetTime();
        //    for (int i = 0; i < 1001; ++i) {
        //        xs1[i] = i * 0.001f;
        //        ys1[i] = 0.5f + 0.5f * sinf(50 * (xs1[i] + (float)DEMO_TIME / 10));
        //    }
        //    static double xs2[11], ys2[11];
        //    for (int i = 0; i < 11; ++i) {
        //        xs2[i] = i * 0.1f;
        //        ys2[i] = xs2[i] * xs2[i];
        //    }
        //    ImGui::Begin("Plot");
        //    ImGui::BulletText("Anti-aliasing can be enabled from the plot's context menu (see Help).");
        //    if (ImPlot::BeginPlot("Line Plot", "x", "f(x)")) {
        //        ImPlot::PlotLine("sin(x)", xs1, ys1, 1001);
        //        ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle);
        //        ImPlot::PlotLine("x^2", xs2, ys2, 11);
        //        ImPlot::EndPlot();
        //    }
        //    ImGui::End();
        //}

        //{
        //    DrawJsonConfig("config", Config::get_instance().getDocument());
        //}

        //{
        //    Logger::get_instance().logger->flush();
        //    my_log.AddLog( "%s", Logger::get_instance().oss.str().c_str() );
        //    Logger::get_instance().oss.str("");
        //    Logger::get_instance().oss.clear();
        //    my_log.Draw("Log");
        //}

        {
            const float DISTANCE = 10.0f;
            static float f = 0.0f;
            ImVec2 window_pos = ImVec2(DISTANCE, DISTANCE);
            ImVec2 window_pos_pivot = ImVec2(0.0f, 0.0f);
            ImGui::SetNextWindowPos(window_pos, ImGuiCond_Appearing, window_pos_pivot);
            ImGui::SetNextWindowSize(ImVec2(300,200), ImGuiCond_Always);
            ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
            if (ImGui::Begin("GUI", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav))
            {
                {
                    ImGui::Text("GUI runs at %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
                }
                {
                    ImGui::Text("Image Rendering Mode");
                    ImGui::Indent();
                    ImGui::RadioButton("ImGui", &selectedShowImageMode, SHOW_IMAGE_MODE::IMGUI); ImGui::SameLine();
                    ImGui::RadioButton("OpenCV", &selectedShowImageMode, SHOW_IMAGE_MODE::OPENCV);
                    ImGui::Unindent();
                }
                {
                    static float f = 0.0f;
                    ImGui::Text("Window Capture");
                    ImGui::Indent();
                    if (ImGui::Button("Capture")) {
                        requestedWindowCapture = true;
                    }
                    ImGui::Unindent();
                    ImGui::Text("Window Recording");
                    ImGui::Indent();
                    if (ImGui::Button("Start")) {
                        if (windowRecordingStatus == WINDOW_RECORDING_STATUS::PAUSED) {
                            int fps_encode = 30;
                            windowRecordingFileName = "recording_" + Util::now() + ".mp4";
                            SPDLOG_INFO("Video recording start: {}", windowRecordingFileName);
                            writer = cv::VideoWriter(
                                    Config::get_instance().resultDirectory() + "/" + windowRecordingFileName,
                                    cv::VideoWriter::fourcc('m', 'p', '4', 'v'), fps_encode,
                                    cv::Size((int) io.DisplaySize.x * (int) io.DisplayFramebufferScale.x,
                                             (int) io.DisplaySize.y * (int) io.DisplayFramebufferScale.y));
                            windowRecordingStatus = WINDOW_RECORDING_STATUS::REQUESTED;
                        }
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Stop")) {
                        SPDLOG_INFO("Video recording end");
                        windowRecordingStatus = WINDOW_RECORDING_STATUS::PAUSED;
                        writer.release();
                    }
                    ImGui::SameLine();
                    if (windowRecordingStatus == WINDOW_RECORDING_STATUS::PAUSED) {
                        ImGui::Text("PAUSED");
                    } else if (windowRecordingStatus == WINDOW_RECORDING_STATUS::REQUESTED) {
                        ImGui::Text("REQUESTED");
                    } else if (windowRecordingStatus == WINDOW_RECORDING_STATUS::RECORDING) {
                        ImGui::Text("%s", "RECORDING...");
                    }
                    ImGui::Unindent();
                }
            }
            ImGui::End();

        }

        /// Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);

        /// Window capture and recording
        if (requestedWindowCapture) {
            glFinish();
            int width = (int) io.DisplaySize.x * (int) io.DisplayFramebufferScale.x;
            int height = (int) io.DisplaySize.y * (int) io.DisplayFramebufferScale.y;
            int type = CV_8UC3;
            int format = GL_BGR;
            glReadBuffer(GL_BACK);
            static cv::Mat out_img;
            out_img = cv::Mat(cv::Size(width, height), type);
            glReadPixels(0, 0, width, height, format, GL_UNSIGNED_BYTE, out_img.data);
            cv::flip(out_img, out_img, 0);
            cv::imwrite((Config::get_instance().resultDirectory() + "/capture_" + Util::now() + ".png").c_str(),out_img,std::vector<int>(cv::IMWRITE_PNG_COMPRESSION));
            SPDLOG_INFO("Window captured: {}", "capture_" + Util::now() + ".png");
            requestedWindowCapture = false;
        }
        if (windowRecordingStatus >= WINDOW_RECORDING_STATUS::REQUESTED) {
            /// NOTE: For better color representation, consider using ffmpeg
            windowRecordingStatus = WINDOW_RECORDING_STATUS::RECORDING;
            glFinish();
            int width = (int) io.DisplaySize.x * (int) io.DisplayFramebufferScale.x;
            int height = (int) io.DisplaySize.y * (int) io.DisplayFramebufferScale.y;
            int type = CV_8UC3;
            int format = GL_BGR;
            glReadBuffer(GL_BACK);
            static cv::Mat out_img;
            out_img = cv::Mat(cv::Size(width, height), type);
            glReadPixels(0, 0, width, height, format, GL_UNSIGNED_BYTE, out_img.data);
            cv::flip(out_img, out_img, 0);
            writer << out_img;
        }

        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    appMsg->close();

    return true;
}


