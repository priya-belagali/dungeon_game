#include <iostream>
#include <string>
#include <sstream>
#include <cctype>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#define EXPORT extern "C" EMSCRIPTEN_KEEPALIVE
#else
#define EXPORT extern "C"
#endif

static std::string toLowerStr(std::string str) {
    for (size_t i = 0; i < str.length(); ++i) {
        str[i] = static_cast<char>(::tolower(static_cast<unsigned char>(str[i])));
    }
    return str;
}

static std::string trimStr(const std::string& s) {
    size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) ++start;
    size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) --end;
    return s.substr(start, end - start);
}

class Player; // Forward declaration

// ==========================================
// 1. INHERITANCE & ABSTRACTION: Item Hierarchy
// ==========================================
class Item {
protected:
    std::string name;
    std::string description;

public:
    Item() : name("Empty"), description("None") {}
    Item(std::string n, std::string d) : name(n), description(d) {}
    virtual ~Item() = default;

    std::string getName() const { return name; }
    std::string getDescription() const { return description; }

    virtual bool use(Player& player, std::ostringstream& out) = 0;
};

class TorchItem : public Item {
public:
    TorchItem() : Item("Torch", "A flaming torch that reveals traps and hidden paths.") {}
    bool use(Player&, std::ostringstream& out) override {
        out << "[Item Used] The flame flares bright, pushing back the dungeon shadows.\n";
        return false;
    }
};

class HerbItem : public Item {
private:
    int healAmount;

public:
    HerbItem(int heal = 30)
        : Item("Herb", "A glowing moss herb that restores 30 HP."), healAmount(heal) {}

    int getHealAmount() const { return healAmount; }
    bool use(Player& player, std::ostringstream& out) override;
};

class KeyItem : public Item {
public:
    KeyItem() : Item("Key", "A heavy brass key etched with ancient runes.") {}
    bool use(Player&, std::ostringstream& out) override {
        out << "[Item Used] The brass runes hum softly—it belongs to a heavy lock.\n";
        return false;
    }
};

// ==========================================
// 2. ENCAPSULATION: Inventory, Entity & Player
// ==========================================
class Inventory {
private:
    static const int MAX_ITEMS = 6;
    Item* items[MAX_ITEMS];
    int itemCount;

public:
    Inventory() : itemCount(0) {
        for (int i = 0; i < MAX_ITEMS; ++i) {
            items[i] = nullptr;
        }
    }

    bool addItem(Item* item, std::ostringstream& out) {
        if (hasItem(item->getName())) {
            out << "\n[!] You already carry a " << item->getName() << ".\n";
            return false;
        }
        if (itemCount < MAX_ITEMS) {
            items[itemCount++] = item;
            out << "\n[+] Added to Inventory: " << item->getName() << "\n";
            return true;
        }
        out << "\n[!] Inventory is full!\n";
        return false;
    }

    bool hasItem(const std::string& itemName) const {
        for (int i = 0; i < itemCount; ++i) {
            if (items[i] != nullptr && toLowerStr(items[i]->getName()) == toLowerStr(itemName)) {
                return true;
            }
        }
        return false;
    }

    Item* getItem(const std::string& itemName) {
        for (int i = 0; i < itemCount; ++i) {
            if (items[i] != nullptr && toLowerStr(items[i]->getName()) == toLowerStr(itemName)) {
                return items[i];
            }
        }
        return nullptr;
    }

    bool removeItem(const std::string& itemName) {
        for (int i = 0; i < itemCount; ++i) {
            if (items[i] != nullptr && toLowerStr(items[i]->getName()) == toLowerStr(itemName)) {
                for (int j = i; j < itemCount - 1; ++j) {
                    items[j] = items[j + 1];
                }
                items[itemCount - 1] = nullptr;
                itemCount--;
                return true;
            }
        }
        return false;
    }

    void display(std::ostringstream& out) const {
        out << "\n--- INVENTORY ---\n";
        if (itemCount == 0) {
            out << "Empty.\n";
        } else {
            for (int i = 0; i < itemCount; ++i) {
                if (items[i] != nullptr) {
                    out << "- " << items[i]->getName() << ": " << items[i]->getDescription() << "\n";
                }
            }
        }
        out << "-----------------\n";
    }
};

class Entity {
protected:
    std::string name;
    int health;
    int maxHealth;

public:
    Entity(std::string n, int hp) : name(n), health(hp), maxHealth(hp) {}
    virtual ~Entity() = default;

    std::string getName() const { return name; }
    int getHealth() const { return health; }
    int getMaxHealth() const { return maxHealth; }
    bool isAlive() const { return health > 0; }

    virtual void takeDamage(int amount, std::ostringstream& out) {
        health -= amount;
        if (health < 0) health = 0;
        out << " [!] " << name << " took " << amount << " damage! HP: " << health << "/" << maxHealth << "\n";
    }

    virtual void heal(int amount, std::ostringstream& out) {
        health += amount;
        if (health > maxHealth) health = maxHealth;
        out << " [+] " << name << " recovered " << amount << " HP! HP: " << health << "/" << maxHealth << "\n";
    }
};

class Player : public Entity {
private:
    Inventory inventory;

public:
    Player(std::string name) : Entity(name, 100) {}

    Inventory& getInventory() { return inventory; }
    const Inventory& getInventory() const { return inventory; }

    void useItem(const std::string& itemName, std::ostringstream& out) {
        Item* item = inventory.getItem(itemName);
        if (item == nullptr) {
            out << "\n[!] You don't have '" << itemName << "'.\n";
            return;
        }
        bool consumed = item->use(*this, out);
        if (consumed) {
            inventory.removeItem(itemName);
        }
    }
};

bool HerbItem::use(Player& player, std::ostringstream& out) {
    player.heal(healAmount, out);
    return true;
}

// ==========================================
// 3. POLYMORPHISM: Obstacle & Room Graph
// ==========================================
class Obstacle {
protected:
    std::string name;
    bool resolved;

public:
    Obstacle(std::string n) : name(n), resolved(false) {}
    virtual ~Obstacle() = default;

    std::string getName() const { return name; }
    bool isResolved() const { return resolved; }
    void resetResolved() { resolved = false; }

    virtual bool interact(Player& player, std::ostringstream& out) = 0;
};

class FloorTrap : public Obstacle {
public:
    FloorTrap() : Obstacle("Floor Pressure Plate") {}

    bool interact(Player& player, std::ostringstream& out) override {
        if (!player.getInventory().hasItem("Torch")) {
            out << "\nIt's pitch black! You stumble over a raised stone and trip a trap!\n";
            player.takeDamage(20, out);
            return false;
        }

        if (!resolved) {
            out << "\n[Obstacle] A floor tile sinks! Poison darts whistle from the wall!\n";
            out << "Type [dodge] or [block].\n";
            return false;
        }

        out << "\nYou step past the already-triggered pressure plate.\n";
        return true;
    }

    void resolveChoice(Player& player, const std::string& choice, std::ostringstream& out) {
        if (toLowerStr(choice) == "dodge") {
            out << "You roll cleanly beneath the dart line!\n";
        } else {
            out << "A dart grazes your guard!\n";
            player.takeDamage(20, out);
        }
        resolved = true;
    }
};

class CreviceSearch : public Obstacle {
private:
    KeyItem* keyRef;

public:
    CreviceSearch(KeyItem* key) : Obstacle("Wall Crevice"), keyRef(key) {}

    bool interact(Player& player, std::ostringstream& out) override {
        if (resolved || player.getInventory().hasItem("Key")) {
            out << "\nThe crevice has already been searched.\n";
            return true;
        }
        out << "\n[Obstacle] A narrow fissure glints inside. Type [left] or [right].\n";
        return false;
    }

    bool resolveChoice(Player& player, const std::string& choice, std::ostringstream& out) {
        if (toLowerStr(choice) == "right") {
            out << "Your fingers close around a heavy brass key!\n";
            player.getInventory().addItem(keyRef, out);
            resolved = true;
            return true;
        }
        out << "A cave spider nips your hand before scurrying away!\n";
        player.takeDamage(20, out);
        return false;
    }
};

class RiddleDoor : public Obstacle {
public:
    RiddleDoor() : Obstacle("Runed Lock Gate") {}

    bool interact(Player& player, std::ostringstream& out) override {
        if (!player.getInventory().hasItem("Key")) {
            out << "\nThe iron gate is sealed tight. You need a Key.\n";
            return false;
        }
        out << "\n[Obstacle] You turn the brass Key. A carved gargoyle speaks:\n";
        out << " 'What gets wetter the more it dries?' Type your answer.\n";
        return false;
    }

    bool resolveAnswer(Player& player, const std::string& answer, std::ostringstream& out) {
        std::string clean = toLowerStr(trimStr(answer));
        if (clean == "towel" || clean == "a towel") {
            out << "\nStone gears grind open, revealing cool night air and freedom!\n";
            resolved = true;
            return true;
        }
        out << "\nA shockwave of runes blasts back from the lock!\n";
        player.takeDamage(30, out);
        return false;
    }
};

class Room {
private:
    static const int MAX_OBSTACLES = 3;
    std::string id;
    std::string title;
    std::string description;
    Obstacle* obstacles[MAX_OBSTACLES];
    int obstacleCount;
    Item* roomItem;
    Room* northRoom;
    Room* southRoom;

public:
    Room(std::string roomId, std::string t, std::string desc, Item* item = nullptr)
        : id(roomId), title(t), description(desc), obstacleCount(0), roomItem(item),
          northRoom(nullptr), southRoom(nullptr) {
        for (int i = 0; i < MAX_OBSTACLES; ++i) {
            obstacles[i] = nullptr;
        }
    }

    const std::string& getId() const { return id; }

    void setExits(Room* north, Room* south) {
        northRoom = north;
        southRoom = south;
    }

    Room* getNorth() const { return northRoom; }
    Room* getSouth() const { return southRoom; }

    bool addObstacle(Obstacle* obs) {
        if (obstacleCount < MAX_OBSTACLES) {
            obstacles[obstacleCount++] = obs;
            return true;
        }
        return false;
    }

    Item* takeRoomItem() {
        Item* temp = roomItem;
        roomItem = nullptr;
        return temp;
    }

    void describe(std::ostringstream& out, int playerHp, int maxHp) const {
        out << "\n======================================================\n";
        out << title << "  |  HP: " << playerHp << "/" << maxHp << "\n";
        out << "------------------------------------------------------\n";
        out << description << "\n";
        if (roomItem != nullptr) {
            out << "You spot a [" << roomItem->getName() << "] nearby.\n";
        }
        if (northRoom != nullptr) out << "Path [north] leads onward.\n";
        if (southRoom != nullptr) out << "Path [south] leads back.\n";
        out << "======================================================\n";
    }
};

// ==========================================
// 4. COMPOSITION & ABSTRACTION: Game Engine
// ==========================================
class GameEngine {
private:
    Player player;
    TorchItem torchItem;
    HerbItem herbItem;
    KeyItem keyItem;

    FloorTrap floorTrap;
    CreviceSearch creviceSearch;
    RiddleDoor riddleDoor;

    Room cellRoom;
    Room corridorRoom;
    Room gateRoom;
    Room* currentRoom;
    bool escaped;
    std::string pendingPrompt; // "", "trap", "crevice", "riddle"

public:
    GameEngine()
        : player("Hero"),
          torchItem(),
          herbItem(30),
          keyItem(),
          floorTrap(),
          creviceSearch(&keyItem),
          riddleDoor(),
          cellRoom(
              "cell",
              "Damp Dungeon Cell",
              "Cold water drips from cracked stone. A rusted torch sits near the wall.",
              &torchItem
          ),
          corridorRoom(
              "corridor",
              "Trapped Gallery",
              "A narrow stone hall lined with dart slits and wall fissures. Glowing moss grows near the floor.",
              &herbItem
          ),
          gateRoom(
              "gate",
              "Moonlit Threshold",
              "A towering runed gate blocks the exit stairs to the surface."
          ),
          currentRoom(&cellRoom),
          escaped(false),
          pendingPrompt("")
    {
        cellRoom.setExits(&corridorRoom, nullptr);
        corridorRoom.setExits(&gateRoom, &cellRoom);
        gateRoom.setExits(nullptr, &corridorRoom);

        corridorRoom.addObstacle(&floorTrap);
        corridorRoom.addObstacle(&creviceSearch);
        gateRoom.addObstacle(&riddleDoor);
    }

    std::string step(const std::string& rawInput) {
        std::ostringstream out;
        std::string input = trimStr(rawInput);
        std::string lower = toLowerStr(input);

        if (!player.isAlive()) {
            out << "\n*** YOU FELL IN THE DUNGEON. GAME OVER. ***\n";
            return out.str();
        }

        if (escaped) {
            out << "\n*** YOU ALREADY ESCAPED THE DUNGEON ALIVE! ***\n";
            return out.str();
        }

        if (pendingPrompt == "trap") {
            pendingPrompt = "";
            floorTrap.resolveChoice(player, lower, out);
            if (!player.isAlive()) {
                out << "\n*** YOU FELL IN THE DUNGEON. GAME OVER. ***\n";
                return out.str();
            }
            if (!creviceSearch.isResolved() && !player.getInventory().hasItem("Key")) {
                pendingPrompt = "crevice";
                out << "\n[Obstacle] A narrow fissure glints inside. Type [left] or [right].\n";
                return out.str();
            }
            currentRoom->describe(out, player.getHealth(), player.getMaxHealth());
            return out.str();
        }

        if (pendingPrompt == "crevice") {
            pendingPrompt = "";
            creviceSearch.resolveChoice(player, lower, out);
            if (!player.isAlive()) {
                out << "\n*** YOU FELL IN THE DUNGEON. GAME OVER. ***\n";
                return out.str();
            }
            currentRoom->describe(out, player.getHealth(), player.getMaxHealth());
            return out.str();
        }

        if (pendingPrompt == "riddle") {
            pendingPrompt = "";
            if (riddleDoor.resolveAnswer(player, input, out)) {
                escaped = true;
                out << "\n*** YOU ESCAPED THE DUNGEON ALIVE! ***\n";
                return out.str();
            }
            if (!player.isAlive()) {
                out << "\n*** YOU FELL IN THE DUNGEON. GAME OVER. ***\n";
                return out.str();
            }
            currentRoom->describe(out, player.getHealth(), player.getMaxHealth());
            return out.str();
        }

        if (lower == "take") {
            Item* found = currentRoom->takeRoomItem();
            if (found != nullptr) {
                player.getInventory().addItem(found, out);
            } else {
                out << "\nNothing to pick up here.\n";
            }
        } else if (lower.rfind("use ", 0) == 0) {
            std::string itemName = trimStr(input.substr(4));
            player.useItem(itemName, out);
        } else if (lower == "search") {
            if (currentRoom == &corridorRoom) {
                if (!player.getInventory().hasItem("Torch")) {
                    floorTrap.interact(player, out);
                } else if (!floorTrap.isResolved()) {
                    pendingPrompt = "trap";
                    out << "\n[Obstacle] A floor tile sinks! Poison darts whistle from the wall!\n";
                    out << "Type [dodge] or [block].\n";
                    return out.str();
                } else if (!creviceSearch.isResolved() && !player.getInventory().hasItem("Key")) {
                    pendingPrompt = "crevice";
                    out << "\n[Obstacle] A narrow fissure glints inside. Type [left] or [right].\n";
                    return out.str();
                } else {
                    out << "\nYou have already searched the gallery.\n";
                }
            } else {
                out << "\nNothing unusual to search here.\n";
            }
        } else if (lower == "door") {
            if (currentRoom == &gateRoom) {
                if (!player.getInventory().hasItem("Key")) {
                    riddleDoor.interact(player, out);
                } else {
                    pendingPrompt = "riddle";
                    out << "\n[Obstacle] You turn the brass Key. A carved gargoyle speaks:\n";
                    out << " 'What gets wetter the more it dries?' Type your answer.\n";
                    return out.str();
                }
            } else {
                out << "\nThere is no locked exit gate in this room.\n";
            }
        } else if (lower == "north") {
            if (currentRoom->getNorth() != nullptr) {
                currentRoom = currentRoom->getNorth();
            } else {
                out << "\nSolid stone blocks the way north.\n";
            }
        } else if (lower == "south") {
            if (currentRoom->getSouth() != nullptr) {
                currentRoom = currentRoom->getSouth();
            } else {
                out << "\nThere is no path south.\n";
            }
        } else if (lower == "inventory") {
            player.getInventory().display(out);
        } else if (lower == "look") {
            // Re-describes below
        } else {
            out << "\nCommands: take, use <item>, search, door, north, south, inventory, look\n";
        }

        if (!player.isAlive()) {
            out << "\n*** YOU FELL IN THE DUNGEON. GAME OVER. ***\n";
            return out.str();
        }

        currentRoom->describe(out, player.getHealth(), player.getMaxHealth());
        return out.str();
    }

    std::string getRoomId() const { return currentRoom->getId(); }
    int getHp() const { return player.getHealth(); }
    bool hasTorch() const { return player.getInventory().hasItem("Torch"); }
};

static GameEngine g_engine;

EXPORT const char* run_command(const char* input) {
    static std::string response;
    response = g_engine.step(input ? input : "");
    return response.c_str();
}

EXPORT const char* current_room() {
    static std::string roomId;
    roomId = g_engine.getRoomId();
    return roomId.c_str();
}

EXPORT int current_hp() {
    return g_engine.getHp();
}

EXPORT int has_torch_flag() {
    return g_engine.hasTorch() ? 1 : 0;
}
