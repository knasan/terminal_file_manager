// main.cpp
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>
#include <iostream>

int main() {
    using namespace ftxui;

    // Erstellt ein einfaches Element
    Element document = 
        vbox({
            text("Hallo Welt!") | bold | color(Color::Green),
            separator(),
            text("FTXUI läuft erfolgreich.")
        }) | border;

    // Erstellt und rendert den Screen
    auto screen = Screen::Create(
        Dimension::Full(),
        Dimension::Fit(document)
    );
    Render(screen, document);
    screen.Print();
    
    return 0;
}