#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hidapi/hidapi.h>
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <assert.h>
#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"
#include "icon.h"
#include <iostream>
//#include <ncurses.h>
//SUPER IMPORTANT: https://github.com/Ryochan7/DS4Windows/blob/jay/DS4Windows/DS4Library/InputDevices/DualSenseDevice.cs
//TODO: increase array length to 78 for bluetooth
//TODO: implement CRC for bluetooth

//WISH: autodetect bluetooth

enum dualsense_modes{
    Off = 0x0, //# no resistance
    Rigid = 0x1, //# continous resistance
    Pulse = 0x2, //# section resistance
    Rigid_A = 0x1 | 0x20,
    Rigid_B = 0x1 | 0x04,
    Rigid_AB = 0x1 | 0x20 | 0x04,
    Pulse_A = 0x2 | 0x20,
    Pulse_B = 0x2 | 0x04,
    Pulse_AB = 0x2 | 0x20 | 0x04,
};

void error_sound(){
	std::cout << '\a' << std::flush;
}

int get_mode(int index){
	switch(index){
	case 0:
		return dualsense_modes::Off;
	case 1:
		return dualsense_modes::Rigid;
	case 2:
		return dualsense_modes::Pulse;
	case 3:
		return dualsense_modes::Rigid_A;
	case 4:
		return dualsense_modes::Rigid_B;
	case 5:
		return dualsense_modes::Rigid_AB;
	case 6:
		return dualsense_modes::Pulse_A;
	case 7:
		return dualsense_modes::Pulse_B;
	case 8:
		return dualsense_modes::Pulse_AB;
	default:
		break;
	}
	return 0;
}

int main(int argc, char **argv) {
	hid_init();
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK);
	uint32_t WindowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
	SDL_Window *window = SDL_CreateWindow("Trigger Controls", 0, 0, 640, 480, WindowFlags);
	assert(window);
	SDL_GLContext context = SDL_GL_CreateContext(window);
	SDL_GL_MakeCurrent(window, context);
	SDL_Surface* surface;
	surface = SDL_CreateRGBSurfaceWithFormatFrom(gimp_image.pixel_data, gimp_image.width, gimp_image.height, gimp_image.bytes_per_pixel * 8, 4 *  gimp_image.width,SDL_PIXELFORMAT_RGBA32 );	SDL_SetWindowIcon(window, surface);

	  // ...and the surface containing the icon pixel data is no longer required.
	  SDL_FreeSurface(surface);
	glewExperimental=true;
	glewInit();
	IMGUI_CHECKVERSION();
	    ImGui::CreateContext();
	    ImGuiIO& io = ImGui::GetIO(); (void)io;
	    ImGui::StyleColorsDark();

	       // setup platform/renderer bindings
	    ImGui_ImplSDL2_InitForOpenGL(window, context);
	    ImGui_ImplOpenGL3_Init("#version 150");
	struct hid_device_info *devs, *cur_dev;
		devs = hid_enumerate(0x0,0x0);
		cur_dev = devs;
		bool bt = false;
		char* path = NULL;
		while (cur_dev) {
			if(cur_dev->vendor_id == 0x054c && cur_dev->product_id == 0x0ce6){
				path = (char*)calloc(strlen(cur_dev->path), sizeof(char));
				memcpy(path, cur_dev->path, strlen(cur_dev->path) * sizeof(char));
				break;
			}
				cur_dev = cur_dev->next;
			}
			hid_free_enumeration(devs);
		hid_device *handle = hid_open_path(path);
		if(handle == NULL){
			error_sound();
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,"ERROR","could not find a dualsense controller!",window);
			exit(EXIT_FAILURE);
		}
		printf("%d\n",bt);
		free(path);
		bool running = true;
		//SDL_SetWindowResizable(window, SDL_bool::SDL_TRUE);
		uint8_t* outReport = new uint8_t[65];
	    memset(outReport, 0, 65);
	    outReport[0] = 0x2;
	    outReport[1] = 0x04 | 0x08;
		outReport[2] = 0x40;
		const char* states[9] = {"Off","Rigid","Pulse","RigidA","RigidB","RigidAB","PulseA","PulseB","PulseAB"};
		int left_cur = 0;
		int right_cur = 0;
	while(running){

		 SDL_Event event;
		    while (SDL_PollEvent(&event))
		    {
		    	 ImGui_ImplSDL2_ProcessEvent(&event);
		    	if (event.type == SDL_QUIT)
		        {
		    	   running = false;
		        }
		    }


		int height, width;
		SDL_GetWindowSize(window, &width, &height);
	    glViewport(0, 0, width, height);
	    glClearColor(0.f, 0.f, 0.f, 0.f);
	    glClear(GL_COLOR_BUFFER_BIT);
	    ImGui_ImplOpenGL3_NewFrame();
	    ImGui_ImplSDL2_NewFrame(window);
	    ImGui::NewFrame();
        ImGui::SetNextWindowSize(
            ImVec2(static_cast<float>(width), static_cast<float>(height)),
            ImGuiCond_Always
            );
        ImGui::SetNextWindowPos(ImVec2(0,0), ImGuiCond_Always, ImVec2(0,0));
	    ImGui::Begin("Controls", NULL,ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar);

	    ImGui::Checkbox("Using Bluetooth?",&bt);

	    if(ImGui::Button("Reset")){
		    memset(outReport, 0, 65);
		    if(!bt)
		    outReport[0] = 0x2;
		    if(bt)
		    outReport[0] = 0x31; //thx ds4windows
		    outReport[1] = 0x04 | 0x08;
			outReport[2] = 0x40;
			hid_write(handle,outReport,65);
			left_cur = 0;
			right_cur = 0;
			printf("reset!\n");
	    }

	    ImGui::Text("Right Trigger:");
	    ImGui::ListBox("Right Mode", &right_cur, states, IM_ARRAYSIZE(states));
	    outReport[11] = get_mode(right_cur);
	    int arr[7] = {0};
	    for(int i = 0; i < 6; i++){
	    	 arr[i] = outReport[i + 12];
	    }
	    arr[6] = outReport[20];
	    ImGui::SliderInt("Right Start Resistance", (int*)&arr[0], 0, 255, "%d", 0);
	    ImGui::SliderInt("Right Effect Force", (int*)&arr[1], 0, 255, "%d", 0);
	    ImGui::SliderInt("Right Range Force", (int*)&arr[2], 0, 255, "%d", 0);
	    ImGui::SliderInt("Right Near Release Strength", (int*)&arr[3], 0, 255, "%d", 0);
	    ImGui::SliderInt("Right Near Middle Strength", (int*)&arr[4], 0, 255, "%d", 0);
	    ImGui::SliderInt("Right Pressed Strength", (int*)&arr[5], 0, 255, "%d", 0);
	    ImGui::SliderInt("Right Actuation Frequency", (int*)&arr[6], 0, 255, "%d", 0);
	    for(int i = 0; i < 6; i++){
	    	outReport[i + 12] = arr[i];
	    }
	    outReport[20] = arr[6];
	    ImGui::Text("Left Trigger:");
	    ImGui::ListBox("Left Mode", &left_cur, states, IM_ARRAYSIZE(states));
	    outReport[22] = get_mode(left_cur);
	    int arr2[7] = {0};
	    for(int i = 0; i < 6; i++){
	    	 arr2[i] = outReport[i + 23];
	    }
	    arr2[6] = outReport[31];
	    ImGui::SliderInt("Left Start Resistance", (int*)&arr2[0], 0, 255, "%d", 0);
	    ImGui::SliderInt("Left Effect Force", (int*)&arr2[1], 0, 255, "%d", 0);
	    ImGui::SliderInt("Left Range Force", (int*)&arr2[2], 0, 255, "%d", 0);
	    ImGui::SliderInt("Left Near Release Strength", (int*)&arr2[3], 0, 255, "%d", 0);
	    ImGui::SliderInt("Left Near Middle Strength", (int*)&arr2[4], 0, 255, "%d", 0);
	    ImGui::SliderInt("Left Pressed Strength", (int*)&arr2[5], 0, 255, "%d", 0);
	    ImGui::SliderInt("Left Actuation Frequency", (int*)&arr2[6], 0, 255, "%d", 0);
	    for(int i = 0; i < 6; i++){
	    	outReport[i + 23] = arr2[i];
	    }
	    outReport[31] = arr2[6];
	    if(ImGui::Button("Apply")){
	    	printf("applied!\n");
	    	if(bt)
	    		outReport[0] = 0x31;
	    	else
	    		outReport[0] = 0x02;
	    	hid_write(handle,outReport,65);
	    }

	    ImGui::End();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		SDL_GL_SwapWindow(window);

	}
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);
	//program termination should free memory I forgot to free :D
	return 0;
}