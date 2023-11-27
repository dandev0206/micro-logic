#include <vk2d/vk_instance.h>
#include "main_window.h"
#include "gui/platform/windows_main_window.h"

using namespace std;
using namespace vk2d;

int main() {
	vk::ApplicationInfo info;
	info.pApplicationName = "micro logic";
	info.apiVersion = VK_API_VERSION_1_3;

	VKInstance instance(info, {}, {}, true);

	//Window window(720, 480, "test");

	//while (!window.isClosed()) {
	//	vk2d::Event e;
	//	while (window.pollEvent(e)) {
	//		switch (e.type) {
	//		case Event::Close:
	//			window.close();
	//			break;
	//		case Event::WheelScrolled: {
	//			printf_s("%f\n", e.wheel.delta.y);
	//		} break;
	//		}
	//	}
	//}

	//return 0;

	MainWindow main_window;

	main_window.show();

	return 0;
}