#include "filemanagerui.hpp"

int main() {
  try {
    FileManagerUI ui;
    ui.initialize();
    ui.run();
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
  }
}
