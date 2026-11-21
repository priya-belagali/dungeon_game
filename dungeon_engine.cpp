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
    TorchItem() : Item("Torch", "A blazing torch that pierces darkness and scorches shadow foes.") {}
    bool use(Player&, std::ostringstream& out) override {
        out << "\n[Torch] You raise the flame high! Shadows recoil across the dungeon walls.\n";
        return false;
    }
};

class HerbItem : public Item {
private:
    int healAmount;

public:
    HerbItem(int heal = 35)
        : Item("Herb", "A glowing crimson herb that restores 35 HP."), healAmount(heal) {}

    int getHealAmount() const { return healAmount; }
    bool use(Player& player, std::ostringstream& out) override;
};

class BladeItem : public Item {
private:
    int bonusDamage;

public:
    BladeItem(int dmg = 20)
        : Item("Blade", "A tempered steel shortblade (+20 strike power)."), bonusDamage(dmg) {}

    int getBonusDamage() const { return bonusDamage; }
    bool use(Player&, std::ostringstream& out) override {
        out << "\n[Blade] You test the edge—steel glints sharply in your grip. Use 'attack' during combat!\n";
        return false;
    }
};

class KeyItem : public Item {
public:
    KeyItem() : Item("Key", "An ancient runed key that unlocks the final exit gate.") {}
    bool use(Player&, std::ostringstream& out) override {
        out << "\n[Key] Ancient runes pulse along the brass teeth. Head to the exit gate and type 'door'.\n";
        return false;
    }
};

// ==========================================
// 2. ENCAPSULATION: Inventory, Entity & Player
// ==========================================
class Inventory {
private:
    static const int MAX_ITEMS = 8;
    Item* items[MAX_ITEMS];
    int itemCount;

public:
    Inventory() : itemCount(0) {
        for (int i = 0; i < MAX_ITEMS; ++i) {
            items[i] = nullptr;
        }
    }

    bool addItem(Item* item, std::ostringstream& out) {
        if (item == nullptr) return false;
        if (hasItem(item->getName())) {
            out << "\n[!] You already carry a " << item->getName() << ".\n";
            return false;
        }
        if (itemCount < MAX_ITEMS) {
            items[itemCount++] = item;
            out << "\n[+] Picked up: [" << item->getName() << "] — " << item->getDescription() << "\n";
            return true;
        }
        out << "\n[!] Your pack is full!\n";
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
            out << "Your pack is empty. Try 'take' to pick up items in a room.\n";
        } else {
            for (int i = 0; i < itemCount; ++i) {
                if (items[i] != nullptr) {
                    out << "* " << items[i]->getName() << ": " << items[i]->getDescription() << "\n";
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
        out << " [!] " << name << " takes " << amount << " damage! (" << name << " HP: " << health << "/" << maxHealth << ")\n";
    }

    virtual void heal(int amount, std::ostringstream& out) {
        health += amount;
        if (health > maxHealth) health = maxHealth;
        out << " [+] " << name << " heals +" << amount << " HP! (" << name << " HP: " << health << "/" << maxHealth << ")\n";
    }
};

class Player : public Entity {
private:
    Inventory inventory;
    int baseAttack;

public:
    Player(std::string name) : Entity(name, 120), baseAttack(20) {}

    Inventory& getInventory() { return inventory; }
    const Inventory& getInventory() const { return inventory; }

    int getAttackPower() const {
        int total = baseAttack;
        if (inventory.hasItem("Blade")) total += 20;
        if (inventory.hasItem("Torch")) total += 8;
        return total;
    }

    void useItem(const std::string& itemName, std::ostringstream& out) {
        Item* item = inventory.getItem(itemName);
        if (item == nullptr) {
            out << "\n[!] You don't have '" << itemName << "' in your inventory.\n";
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
// 3. POLYMORPHISM: Enemies & Obstacles
// ==========================================
class Enemy : public Entity {
protected:
    int attackDamage;
    std::string taunt;

public:
    Enemy(std::string n, int hp, int dmg, std::string t)
        : Entity(n, hp), attackDamage(dmg), taunt(t) {}

    int getAttackDamage() const { return attackDamage; }
    std::string getTaunt() const { return taunt; }

    virtual void strikePlayer(Player& player, std::ostringstream& out) {
        out << " [" << name << "] \"" << taunt << "\"\n";
        player.takeDamage(attackDamage, out);
    }
};

class CryptStalker : public Enemy {
public:
    CryptStalker()
        : Enemy("Crypt Stalker", 45, 16, "Fresh bones step into my dark gallery!") {}
};

class ShadowWarden : public Enemy {
public:
    ShadowWarden()
        : Enemy("Shadow Warden", 65, 22, "None pass the Sanctum without answering to the gloom!") {}
};

class Obstacle {
protected:
    std::string name;
    bool resolved;

public:
    Obstacle(std::string n) : name(n), resolved(false) {}
    virtual ~Obstacle() = default;

    std::string getName() const { return name; }
    bool isResolved() const { return resolved; }
    virtual std::string getPromptTag() const = 0;
    virtual void startEncounter(Player& player, std::ostringstream& out) = 0;
    virtual bool handleInput(Player& player, const std::string& input, std::ostringstream& out) = 0;
};

class FloorTrap : public Obstacle {
public:
    FloorTrap() : Obstacle("Dart Pressure Plate") {}
    std::string getPromptTag() const override { return "trap"; }

    void startEncounter(Player& player, std::ostringstream& out) override {
        if (!player.getInventory().hasItem("Torch")) {
            out << "\n[Darkness] Without a Torch, you step blindly onto a raised stone!\n";
            player.takeDamage(15, out);
        }
        out << "\n[Hazard: Dart Pressure Plate] Click 'Dodge' or 'Block' (or type dodge / block).\n";
    }

    bool handleInput(Player& player, const std::string& input, std::ostringstream& out) override {
        std::string c = toLowerStr(trimStr(input));
        if (c == "dodge") {
            out << "\n[Success] You dive low as poison darts zip overhead!\n";
        } else if (c == "block") {
            out << "\n[Guard] Darts rattle off your forearm—minor sting, no poison!\n";
            player.takeDamage(8, out);
        } else {
            out << "\n[!] Hesitation costs you! A dart grazes your shoulder.\n";
            player.takeDamage(14, out);
        }
        resolved = true;
        return true;
    }
};

class CreviceSearch : public Obstacle {
private:
    KeyItem* keyRef;

public:
    CreviceSearch(KeyItem* key) : Obstacle("Runed Wall Crevice"), keyRef(key) {}
    std::string getPromptTag() const override { return "crevice"; }

    void startEncounter(Player&, std::ostringstream& out) override {
        out << "\n[Hazard: Runed Wall Crevice] Something metallic glints inside.\n";
        out << "Click 'Search Right' or 'Search Left' (or type right / left).\n";
    }

    bool handleInput(Player& player, const std::string& input, std::ostringstream& out) override {
        std::string c = toLowerStr(trimStr(input));
        if (c == "right") {
            out << "\n[Found] You reach right and pull out the ancient brass Key!\n";
            player.getInventory().addItem(keyRef, out);
            resolved = true;
            return true;
        }
        out << "\n[Venom] A cave spider snaps from the left gap!\n";
        player.takeDamage(15, out);
        return false;
    }
};

class BrazierPuzzle : public Obstacle {
public:
    BrazierPuzzle() : Obstacle("Twin Flame Braziers") {}
    std::string getPromptTag() const override { return "brazier"; }

    void startEncounter(Player&, std::ostringstream& out) override {
        out << "\n[Puzzle: Twin Flame Braziers] Carved runes read:\n";
        out << "\"Speak the element that lights the dark and chars the shadow\" (type 'fire' or click Fire).\n";
    }

    bool handleInput(Player& player, const std::string& input, std::ostringstream& out) override {
        std::string c = toLowerStr(trimStr(input));
        if (c == "fire" || c == "flame") {
            out << "\n[Puzzle Solved] Golden flame leaps across both braziers, weakening the Warden's shadow armor!\n";
            resolved = true;
            return true;
        }
        out << "\n[Backfire] Cold ash erupts from the braziers!\n";
        player.takeDamage(15, out);
        return false;
    }
};

class RiddleDoor : public Obstacle {
public:
    RiddleDoor() : Obstacle("Astral Exit Gate") {}
    std::string getPromptTag() const override { return "riddle"; }

    void startEncounter(Player& player, std::ostringstream& out) override {
        if (!player.getInventory().hasItem("Key")) {
            out << "\n[Locked] The Astral Exit Gate is sealed tight. Find the [Key] in the Trapped Gallery first!\n";
            return;
        }
        out << "\n[Final Gate] You turn the runed Key. The gate guardian asks:\n";
        out << "\"What gets wetter the more it dries?\" Type 'towel' (or click Towel).\n";
    }

    bool handleInput(Player& player, const std::string& input, std::ostringstream& out) override {
        std::string c = toLowerStr(trimStr(input));
        if (c == "towel" || c == "a towel") {
            out << "\n[VICTORY] Massive stone wings swing open to cool starlight—you escaped the dungeon!\n";
            resolved = true;
            return true;
        }
        out << "\n[Shockwave] Runic lightning arcs from the lock!\n";
        player.takeDamage(20, out);
        return false;
    }
};

// ==========================================
// 4. ROOM GRAPH & GAME ENGINE
// ==========================================
class Room {
private:
    std::string id;
    std::string title;
    std::string objectiveHint;
    std::string description;
    Item* roomItem;
    Enemy* enemy;
    Room* northRoom;
    Room* southRoom;

public:
    Room(std::string roomId, std::string t, std::string hint, std::string desc,
         Item* item = nullptr, Enemy* roomEnemy = nullptr)
        : id(roomId), title(t), objectiveHint(hint), description(desc),
          roomItem(item), enemy(roomEnemy), northRoom(nullptr), southRoom(nullptr) {}

    const std::string& getId() const { return id; }
    const std::string& getTitle() const { return title; }

    void setExits(Room* north, Room* south) {
        northRoom = north;
        southRoom = south;
    }

    Room* getNorth() const { return northRoom; }
    Room* getSouth() const { return southRoom; }
    Enemy* getEnemy() const { return enemy; }

    Item* takeRoomItem() {
        Item* temp = roomItem;
        roomItem = nullptr;
        return temp;
    }

    void describe(std::ostringstream& out, int hp, int maxHp) const {
        out << "\n========================================================\n";
        out << title << "  |  HP: " << hp << "/" << maxHp << "\n";
        out << "Goal: " << objectiveHint << "\n";
        out << "--------------------------------------------------------\n";
        out << description << "\n";
        if (roomItem != nullptr) {
            out << "[Item Here] You spot a [" << roomItem->getName() << "] — click 'Take Item' or type 'take'.\n";
        }
        if (enemy != nullptr && enemy->isAlive()) {
            out << "[Enemy Ahead] " << enemy->getName() << " (HP: " << enemy->getHealth()
                << "/" << enemy->getMaxHealth() << ") blocks the way! Use 'attack'!\n";
        }
        if (northRoom != nullptr) out << "-> North: " << northRoom->getTitle() << "\n";
        if (southRoom != nullptr) out << "-> South: " << southRoom->getTitle() << "\n";
        out << "========================================================\n";
    }
};

class GameEngine {
private:
    Player player;
    TorchItem torchItem;
    BladeItem bladeItem;
    HerbItem herbItem;
    KeyItem keyItem;

    CryptStalker stalker;
    ShadowWarden warden;

    FloorTrap floorTrap;
    CreviceSearch creviceSearch;
    BrazierPuzzle brazierPuzzle;
    RiddleDoor riddleDoor;

    Room cellRoom;
    Room corridorRoom;
    Room sanctumRoom;
    Room gateRoom;

    Room* currentRoom;
    std::string activePrompt; // "", "trap", "crevice", "brazier", "riddle"
    bool escaped;

public:
    GameEngine()
        : player("Adventurer"),
          torchItem(),
          bladeItem(20),
          herbItem(35),
          keyItem(),
          stalker(),
          warden(),
          floorTrap(),
          creviceSearch(&keyItem),
          brazierPuzzle(),
          riddleDoor(),
          cellRoom(
              "cell",
              "1. Damp Dungeon Cell",
              "Pick up the Torch, then head North.",
              "Cold water drips down mossy stones. A bright Torch rests beside the cell bars."
              " Tip: always grab items with 'Take Item' before pushing deeper.",
              &torchItem,
              nullptr
          ),
          corridorRoom(
              "corridor",
              "2. Trapped Gallery",
              "Defeat the Crypt Stalker, grab the Blade, and 'Search Room' for the Key.",
              "A torchlit stone gallery lined with dart slits and a cracked wall crevice."
              " A tempered Blade leans against a pillar.",
              &bladeItem,
              &stalker
          ),
          sanctumRoom(
              "sanctum",
              "3. Ember Sanctum",
              "Solve the brazier puzzle ('fire'), grab the Herb, and defeat the Shadow Warden.",
              "Two unlit braziers flank a shadowy altar where a glowing crimson Herb grows."
              " The Shadow Warden guards the northern archway.",
              &herbItem,
              &warden
          ),
          gateRoom(
              "gate",
              "4. Astral Exit Gate",
              "Unlock with the Key and answer the gate riddle ('towel') to escape!",
              "Cool night air seeps through towering runed doors leading up to freedom."
          ),
          currentRoom(&cellRoom),
          activePrompt(""),
          escaped(false)
    {
        cellRoom.setExits(&corridorRoom, nullptr);
        corridorRoom.setExits(&sanctumRoom, &cellRoom);
        sanctumRoom.setExits(&gateRoom, &corridorRoom);
        gateRoom.setExits(nullptr, &sanctumRoom);
    }

    std::string step(const std::string& rawInput) {
        std::ostringstream out;
        std::string input = trimStr(rawInput);
        std::string lower = toLowerStr(input);

        if (!player.isAlive()) {
            out << "\n*** YOU FELL IN THE DUNGEON. Refresh the page to try a new run! ***\n";
            return out.str();
        }
        if (escaped) {
            out << "\n*** CONGRATULATIONS! You escaped the dungeon alive! ***\n";
            return out.str();
        }

        if (activePrompt == "trap") {
            activePrompt = "";
            floorTrap.handleInput(player, input, out);
            if (!player.isAlive()) {
                out << "\n*** YOU FELL IN THE DUNGEON. Refresh to retry! ***\n";
                return out.str();
            }
            if (!creviceSearch.isResolved() && !player.getInventory().hasItem("Key")) {
                activePrompt = "crevice";
                creviceSearch.startEncounter(player, out);
                return out.str();
            }
            currentRoom->describe(out, player.getHealth(), player.getMaxHealth());
            return out.str();
        }

        if (activePrompt == "crevice") {
            activePrompt = "";
            creviceSearch.handleInput(player, input, out);
            if (!player.isAlive()) {
                out << "\n*** YOU FELL IN THE DUNGEON. Refresh to retry! ***\n";
                return out.str();
            }
            currentRoom->describe(out, player.getHealth(), player.getMaxHealth());
            return out.str();
        }

        if (activePrompt == "brazier") {
            activePrompt = "";
            brazierPuzzle.handleInput(player, input, out);
            if (!player.isAlive()) {
                out << "\n*** YOU FELL IN THE DUNGEON. Refresh to retry! ***\n";
                return out.str();
            }
            currentRoom->describe(out, player.getHealth(), player.getMaxHealth());
            return out.str();
        }

        if (activePrompt == "riddle") {
            activePrompt = "";
            if (riddleDoor.handleInput(player, input, out)) {
                escaped = true;
                out << "\n*** ESCAPE COMPLETE! Score HP remaining: " << player.getHealth() << " ***\n";
                return out.str();
            }
            if (!player.isAlive()) {
                out << "\n*** YOU FELL IN THE DUNGEON. Refresh to retry! ***\n";
                return out.str();
            }
            currentRoom->describe(out, player.getHealth(), player.getMaxHealth());
            return out.str();
        }

        if (lower == "take") {
            Item* item = currentRoom->takeRoomItem();
            if (item != nullptr) {
                player.getInventory().addItem(item, out);
            } else {
                out << "\n[Info] No items left to pick up in this room.\n";
            }
        } else if (lower.rfind("use ", 0) == 0) {
            std::string itemName = trimStr(input.substr(4));
            player.useItem(itemName, out);
        } else if (lower == "attack") {
            Enemy* foe = currentRoom->getEnemy();
            if (foe != nullptr && foe->isAlive()) {
                int dmg = player.getAttackPower();
                if (currentRoom == &sanctumRoom && brazierPuzzle.isResolved()) {
                    dmg += 15;
                    out << "\n[Brazier Bonus] Blazing embers empower your strike (+15 DMG)!\n";
                }
                out << "\nYou strike " << foe->getName() << " for " << dmg << " damage!\n";
                foe->takeDamage(dmg, out);
                if (foe->isAlive()) {
                    foe->strikePlayer(player, out);
                } else {
                    out << "\n[Victory] " << foe->getName() << " is defeated!\n";
                }
            } else {
                out << "\n[Info] There is no enemy to fight in this room.\n";
            }
        } else if (lower == "search") {
            if (currentRoom == &corridorRoom) {
                if (!floorTrap.isResolved()) {
                    activePrompt = "trap";
                    floorTrap.startEncounter(player, out);
                    return out.str();
                } else if (!creviceSearch.isResolved() && !player.getInventory().hasItem("Key")) {
                    activePrompt = "crevice";
                    creviceSearch.startEncounter(player, out);
                    return out.str();
                } else {
                    out << "\n[Info] You've already cleared the traps and crevice here.\n";
                }
            } else if (currentRoom == &sanctumRoom) {
                if (!brazierPuzzle.isResolved()) {
                    activePrompt = "brazier";
                    brazierPuzzle.startEncounter(player, out);
                    return out.str();
                } else {
                    out << "\n[Info] Both braziers are already burning bright.\n";
                }
            } else {
                out << "\n[Info] Nothing hidden to search here. Try 'take' or move 'north'.\n";
            }
        } else if (lower == "door") {
            if (currentRoom == &gateRoom) {
                if (!player.getInventory().hasItem("Key")) {
                    riddleDoor.startEncounter(player, out);
                } else {
                    activePrompt = "riddle";
                    riddleDoor.startEncounter(player, out);
                    return out.str();
                }
            } else {
                out << "\n[Info] The Astral Exit Gate is in Room 4 (keep heading 'north').\n";
            }
        } else if (lower == "north") {
            Enemy* foe = currentRoom->getEnemy();
            if (foe != nullptr && foe->isAlive()) {
                out << "\n[Blocked] " << foe->getName() << " blocks the northern path! Defeat it with 'attack' first.\n";
            } else if (currentRoom->getNorth() != nullptr) {
                currentRoom = currentRoom->getNorth();
            } else {
                out << "\n[Info] No exit north from here.\n";
            }
        } else if (lower == "south") {
            if (currentRoom->getSouth() != nullptr) {
                currentRoom = currentRoom->getSouth();
            } else {
                out << "\n[Info] No path south from here.\n";
            }
        } else if (lower == "inventory") {
            player.getInventory().display(out);
        } else if (lower == "help" || lower == "look") {
            out << "\n[Quick Guide] take | attack | search | use herb | door | north | south | inventory\n";
        } else {
            out << "\n[?] Unknown command '" << input << "'. Try: take, attack, search, use herb, door, north, south, inventory\n";
        }

        if (!player.isAlive()) {
            out << "\n*** YOU FELL IN THE DUNGEON. Refresh the page to try a new run! ***\n";
            return out.str();
        }

        currentRoom->describe(out, player.getHealth(), player.getMaxHealth());
        return out.str();
    }

    std::string getRoomId() const { return currentRoom->getId(); }
    int getHp() const { return player.getHealth(); }
    int getMaxHp() const { return player.getMaxHealth(); }
    int getHasTorch() const { return player.getInventory().hasItem("Torch") ? 1 : 0; }
    int getHasBlade() const { return player.getInventory().hasItem("Blade") ? 1 : 0; }
    int getHasHerb() const { return player.getInventory().hasItem("Herb") ? 1 : 0; }
    int getHasKey() const { return player.getInventory().hasItem("Key") ? 1 : 0; }
    int getEnemyHp() const {
        Enemy* foe = currentRoom->getEnemy();
        return (foe != nullptr && foe->isAlive()) ? foe->getHealth() : 0;
    }
    int getEnemyMaxHp() const {
        Enemy* foe = currentRoom->getEnemy();
        return (foe != nullptr) ? foe->getMaxHealth() : 0;
    }
    const char* getEnemyNameCstr() const {
        static std::string ename;
        Enemy* foe = currentRoom->getEnemy();
        ename = (foe != nullptr && foe->isAlive()) ? foe->getName() : "";
        return ename.c_str();
    }
    const char* getPromptModeCstr() const {
        return activePrompt.c_str();
    }
    int getEscapedFlag() const { return escaped ? 1 : 0; }
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

EXPORT int current_hp() { return g_engine.getHp(); }
EXPORT int max_hp() { return g_engine.getMaxHp(); }
EXPORT int has_torch_flag() { return g_engine.getHasTorch(); }
EXPORT int has_blade_flag() { return g_engine.getHasBlade(); }
EXPORT int has_herb_flag() { return g_engine.getHasHerb(); }
EXPORT int has_key_flag() { return g_engine.getHasKey(); }
EXPORT int enemy_hp() { return g_engine.getEnemyHp(); }
EXPORT int enemy_max_hp() { return g_engine.getEnemyMaxHp(); }
EXPORT const char* enemy_name() { return g_engine.getEnemyNameCstr(); }
EXPORT const char* prompt_mode() { return g_engine.getPromptModeCstr(); }
EXPORT int is_escaped_flag() { return g_engine.getEscapedFlag(); }
