#include "filemanagerui.hpp"

int main() {
  try {
    FileManagerUI ui;
    ui.initialize();
    ui.run();
    // Exception hier? Destruktor wird trotzdem aufgerufen!
  } catch (const std::exception &e) {
    // Terminal ist bereits sauber aufgeräumt
    std::cerr << "Error: " << e.what() << std::endl;
  }
}
