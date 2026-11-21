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
    TorchItem() : Item("Torch", "Blazing idol torch (+8 ATK; scorches shadow & pursuit foes).") {}
    bool use(Player&, std::ostringstream& out) override {
        out << "\n[Torch] You hoist the idol torch—flames lick across the temple carvings!\n";
        return false;
    }
};

class HerbItem : public Item {
private:
    int healAmount;

public:
    HerbItem(int heal = 45)
        : Item("Herb", "Sacred temple leaf that restores 45 HP."), healAmount(heal) {}

    int getHealAmount() const { return healAmount; }
    bool use(Player& player, std::ostringstream& out) override;
};

class BladeItem : public Item {
private:
    int bonusDamage;

public:
    BladeItem(int dmg = 22)
        : Item("Blade", "Tempered temple edge (+22 strike damage)."), bonusDamage(dmg) {}

    int getBonusDamage() const { return bonusDamage; }
    bool use(Player&, std::ostringstream& out) override {
        out << "\n[Blade] You flick your wrist—the temple edge sings in the torchlight.\n";
        return false;
    }
};

class KeyItem : public Item {
public:
    KeyItem() : Item("Key", "Sun-glyph key that unlocks the outer temple gate.") {}
    bool use(Player&, std::ostringstream& out) override {
        out << "\n[Key] The sun-glyph key hums warmly as you hold it.\n";
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
        for (int i = 0; i < MAX_ITEMS; ++i) items[i] = nullptr;
    }

    void reset() {
        itemCount = 0;
        for (int i = 0; i < MAX_ITEMS; ++i) items[i] = nullptr;
    }

    bool addItem(Item* item, std::ostringstream& out) {
        if (item == nullptr) return false;
        if (hasItem(item->getName())) {
            out << "\n[!] You already carry a " << item->getName() << ".\n";
            return false;
        }
        if (itemCount < MAX_ITEMS) {
            items[itemCount++] = item;
            out << "\n[+] Claimed [" << item->getName() << "]: " << item->getDescription() << "\n";
            return true;
        }
        out << "\n[!] Pack is full!\n";
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
                for (int j = i; j < itemCount - 1; ++j) items[j] = items[j + 1];
                items[itemCount - 1] = nullptr;
                itemCount--;
                return true;
            }
        }
        return false;
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

    void setStats(std::string n, int hp) {
        name = n;
        health = hp;
        maxHealth = hp;
    }

    virtual void takeDamage(int amount, std::ostringstream& out) {
        health -= amount;
        if (health < 0) health = 0;
        out << " [!] " << name << " takes " << amount << " DMG (" << health << "/" << maxHealth << " HP)\n";
    }

    virtual void heal(int amount, std::ostringstream& out) {
        health += amount;
        if (health > maxHealth) health = maxHealth;
        out << " [+] " << name << " recovers +" << amount << " HP (" << health << "/" << maxHealth << " HP)\n";
    }
};

class Player : public Entity {
private:
    Inventory inventory;
    int baseAttack;
    int avatarId;
    int stamina;
    int maxStamina;

public:
    Player() : Entity("Kaelen", 130), baseAttack(22), avatarId(1), stamina(100), maxStamina(100) {}

    void configureAvatar(int id) {
        avatarId = id;
        inventory.reset();
        stamina = 100;
        maxStamina = 100;
        if (id == 2) {
            setStats("Lyra", 115);
            baseAttack = 28;
        } else if (id == 3) {
            setStats("Thorne", 150);
            baseAttack = 20;
        } else {
            avatarId = 1;
            setStats("Kaelen", 130);
            baseAttack = 22;
        }
    }

    void reviveAtSanctuary(std::ostringstream& out) {
        health = static_cast<int>(maxHealth * 0.65);
        if (health < 45) health = 45;
        stamina = maxStamina;
        out << "\n[Sanctuary Recovery] You drag yourself back to the Sanctum steps and regain "
            << health << "/" << maxHealth << " HP and full stamina!\n";
    }

    int getAvatarId() const { return avatarId; }
    int getStamina() const { return stamina; }
    int getMaxStamina() const { return maxStamina; }

    bool spendStamina(int cost) {
        if (stamina < cost) return false;
        stamina -= cost;
        return true;
    }

    void gainStamina(int amount) {
        stamina += amount;
        if (stamina > maxStamina) stamina = maxStamina;
    }

    Inventory& getInventory() { return inventory; }
    const Inventory& getInventory() const { return inventory; }

    int getAttackPower() const {
        int total = baseAttack;
        if (inventory.hasItem("Blade")) total += 22;
        if (inventory.hasItem("Torch")) total += 8;
        return total;
    }

    void useItem(const std::string& itemName, std::ostringstream& out) {
        Item* item = inventory.getItem(itemName);
        if (item == nullptr) {
            out << "\n[!] No '" << itemName << "' in your pack.\n";
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
// 3. POLYMORPHISM: Demon Monkeys & Obstacles
// ==========================================
class Enemy : public Entity {
protected:
    int attackDamage;
    std::string taunt;
    int initialHp;
    int initialDmg;

public:
    Enemy(std::string n, int hp, int dmg, std::string t)
        : Entity(n, hp), attackDamage(dmg), taunt(t), initialHp(hp), initialDmg(dmg) {}

    void resetEnemy(int hp, int dmg, std::string t) {
        initialHp = hp;
        initialDmg = dmg;
        health = hp;
        maxHealth = hp;
        attackDamage = dmg;
        taunt = t;
    }

    void partialHealOnLoss() {
        health = static_cast<int>(maxHealth * 0.75);
        if (health < 25) health = 25;
    }

    int getAttackDamage() const { return attackDamage; }

    virtual void strikePlayer(Player& player, std::ostringstream& out, bool guarded = false) {
        out << " [" << name << "] \"" << taunt << "\"\n";
        int dmg = guarded ? (attackDamage / 2) : attackDamage;
        if (guarded) {
            out << " [Guard] Your stance cuts the demon's lunge in half!\n";
        }
        player.takeDamage(dmg, out);
    }
};

class CrimsonHowler : public Enemy {
public:
    CrimsonHowler()
        : Enemy("Crimson Demon Monkey", 60, 19, "Screech! Another runner slips on the wet flagstones!") {}
};

class FangBrute : public Enemy {
public:
    FangBrute()
        : Enemy("Fang Demon Monkey", 75, 22, "Chatter-snarl! Nowhere left to vault in this corridor!") {}
};

class EmberAlpha : public Enemy {
public:
    EmberAlpha()
        : Enemy("Ember Demon Monkey", 90, 25, "Roar-shriek! The temple's cursed pack hunts as one!") {}
};

class IdolColossus : public Enemy {
public:
    IdolColossus()
        : Enemy("Alpha Idol Monkey", 115, 29, "THUNDEROUS SCREECH! Drop the idol or fall beneath the pack!") {}
};

class Obstacle {
protected:
    std::string name;
    bool resolved;

public:
    Obstacle(std::string n) : name(n), resolved(false) {}
    virtual ~Obstacle() = default;

    void resetResolved() { resolved = false; }
    bool isResolved() const { return resolved; }
    virtual void startEncounter(Player& player, std::ostringstream& out) = 0;
    virtual bool handleInput(Player& player, const std::string& input, std::ostringstream& out) = 0;
};

class FloorTrap : public Obstacle {
public:
    FloorTrap() : Obstacle("Spike & Dart Run") {}

    void startEncounter(Player& player, std::ostringstream& out) override {
        if (!player.getInventory().hasItem("Torch")) {
            out << "\n[Shadow Slip] Without a Torch, you clip a mossy trip-stone!\n";
            player.takeDamage(16, out);
        }
        out << "\n[Run Hazard] Floor spikes snap ahead while darts hiss from the wall! Choose your move: slide, leap, or brace.\n";
    }

    bool handleInput(Player& player, const std::string& input, std::ostringstream& out) override {
        std::string c = toLowerStr(trimStr(input));
        if (c == "slide" || c == "dodge" || c == "leap") {
            out << "\n[Clean Run] You drop into a low temple slide beneath the volley!\n";
        } else if (c == "brace" || c == "block") {
            out << "\n[Glancing Hit] You brace your arm—light scrape, no poison.\n";
            player.takeDamage(9, out);
        } else {
            out << "\n[Stumble] Late footwork catches a dart on the shoulder!\n";
            player.takeDamage(16, out);
        }
        resolved = true;
        return true;
    }
};

class CreviceSearch : public Obstacle {
private:
    KeyItem* keyRef;

public:
    CreviceSearch(KeyItem* key) : Obstacle("Carved Idol Niche"), keyRef(key) {}

    void startEncounter(Player&, std::ostringstream& out) override {
        out << "\n[Niche] A sun-glyph key glints inside a split stone wall. Which side do you reach into?\n";
    }

    bool handleInput(Player& player, const std::string& input, std::ostringstream& out) override {
        std::string c = toLowerStr(trimStr(input));
        if (c == "right") {
            out << "\n[Claimed] You reach right and snatch the sun-glyph Key!\n";
            player.getInventory().addItem(keyRef, out);
            resolved = true;
            return true;
        }
        out << "\n[Ambush] A venomous temple crawler snaps from that side!\n";
        player.takeDamage(16, out);
        return false;
    }
};

class CisternRiddle : public Obstacle {
public:
    CisternRiddle() : Obstacle("Sunken Gong Shrine") {}

    void startEncounter(Player&, std::ostringstream& out) override {
        out << "\n[Riddle I] Water ripples around a bronze gong as runes glow:\n";
        out << "\"I speak without a mouth and hear without ears. I have no body, but I come alive with wind. What am I?\"\n";
    }

    bool handleInput(Player& player, const std::string& input, std::ostringstream& out) override {
        std::string c = toLowerStr(trimStr(input));
        if (c == "echo" || c == "an echo") {
            out << "\n[Solved] Stone stepping-stones rise above the floodwater!\n";
            resolved = true;
            return true;
        }
        out << "\n[Wrong] Freezing spray lashes across the flooded hall!\n";
        player.takeDamage(17, out);
        return false;
    }
};

class BrazierPuzzle : public Obstacle {
public:
    BrazierPuzzle() : Obstacle("Twin Flame Braziers") {}

    void startEncounter(Player&, std::ostringstream& out) override {
        out << "\n[Riddle II] Carved temple runes read:\n";
        out << "\"I consume forests, drink rivers, and fear no blade—yet a single raindrop can end me. What am I?\"\n";
    }

    bool handleInput(Player& player, const std::string& input, std::ostringstream& out) override {
        std::string c = toLowerStr(trimStr(input));
        if (c == "fire" || c == "flame") {
            out << "\n[Solved] Golden flame leaps across both braziers, scorching nearby shadow foes!\n";
            resolved = true;
            return true;
        }
        out << "\n[Wrong] Cold ash bursts from the unlit braziers!\n";
        player.takeDamage(17, out);
        return false;
    }
};

class ForgeRiddle : public Obstacle {
public:
    ForgeRiddle() : Obstacle("Obsidian Anvil Seal") {}

    void startEncounter(Player&, std::ostringstream& out) override {
        out << "\n[Riddle III] Glowing runes pulse across the obsidian anvil:\n";
        out << "\"The more you take from me, the bigger I grow. What am I?\"\n";
    }

    bool handleInput(Player& player, const std::string& input, std::ostringstream& out) override {
        std::string c = toLowerStr(trimStr(input));
        if (c == "hole" || c == "a hole") {
            out << "\n[Solved] The obsidian seal cracks open in a shower of sparks!\n";
            resolved = true;
            return true;
        }
        out << "\n[Wrong] Molten cinders spurt from the anvil!\n";
        player.takeDamage(18, out);
        return false;
    }
};

class RiddleDoor : public Obstacle {
public:
    RiddleDoor() : Obstacle("Outer Temple Gate") {}

    void startEncounter(Player& player, std::ostringstream& out) override {
        if (!player.getInventory().hasItem("Key")) {
            out << "\n[Sealed] The Outer Temple Gate requires the [Key] from the Trapped Gallery.\n";
            return;
        }
        out << "\n[Riddle IV — Final Gate] The stone guardian speaks:\n";
        out << "\"What gets wetter the more it dries?\"\n";
    }

    bool handleInput(Player& player, const std::string& input, std::ostringstream& out) override {
        std::string c = toLowerStr(trimStr(input));
        if (c == "towel" || c == "a towel") {
            out << "\n[ESCAPED] Heavy temple doors thunder open to sunlight and jungle air!\n";
            resolved = true;
            return true;
        }
        out << "\n[Wrong] Shock-runes flash across the gate lock!\n";
        player.takeDamage(22, out);
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
    Item* initialItem;
    Enemy* enemy;
    Room* northRoom;
    Room* southRoom;

public:
    Room(std::string roomId, std::string t, std::string hint, std::string desc,
         Item* item = nullptr, Enemy* roomEnemy = nullptr)
        : id(roomId), title(t), objectiveHint(hint), description(desc),
          roomItem(item), initialItem(item), enemy(roomEnemy),
          northRoom(nullptr), southRoom(nullptr) {}

    const std::string& getId() const { return id; }
    const std::string& getTitle() const { return title; }
    const std::string& getObjective() const { return objectiveHint; }

    void resetItem() { roomItem = initialItem; }

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

    void describe(std::ostringstream& out, int hp, int maxHp, int stam) const {
        out << "\n[" << title << "] HP " << hp << "/" << maxHp << " | STM " << stam << " — " << objectiveHint << "\n";
        out << description << "\n";
        if (roomItem != nullptr) {
            out << " * Relic: [" << roomItem->getName() << "] ('take')\n";
        }
        if (enemy != nullptr && enemy->isAlive()) {
            out << " * Demon Monkey: " << enemy->getName() << " (" << enemy->getHealth()
                << "/" << enemy->getMaxHealth() << " HP) — moves: slash, vault, guard, torch\n";
        }
    }
};

class GameEngine {
private:
    Player player;
    TorchItem torchItem;
    BladeItem bladeItem;
    HerbItem herbItem1;
    HerbItem herbItem2;
    KeyItem keyItem;

    CrimsonHowler howler;
    FangBrute brute;
    EmberAlpha emberAlpha;
    IdolColossus colossus;

    FloorTrap floorTrap;
    CreviceSearch creviceSearch;
    CisternRiddle cisternRiddle;
    BrazierPuzzle brazierPuzzle;
    ForgeRiddle forgeRiddle;
    RiddleDoor riddleDoor;

    Room cellRoom;
    Room corridorRoom;
    Room cisternRoom;
    Room forgeRoom;
    Room sanctumRoom;
    Room gateRoom;

    Room* currentRoom;
    std::string activePrompt;
    std::string lastAnimAction;
    int difficulty;
    int defeatsCount;
    bool escaped;

public:
    GameEngine()
        : player(),
          torchItem(),
          bladeItem(22),
          herbItem1(45),
          herbItem2(45),
          keyItem(),
          howler(),
          brute(),
          emberAlpha(),
          colossus(),
          floorTrap(),
          creviceSearch(&keyItem),
          cisternRiddle(),
          brazierPuzzle(),
          forgeRiddle(),
          riddleDoor(),
          cellRoom("cell", "1. Idol Courtyard", "Claim Torch, then sprint North.",
                   "Mossy temple steps lead past a flickering Torch toward the demon-haunted corridor.", &torchItem, nullptr),
          corridorRoom("corridor", "2. Trapped Gallery", "Fight Crimson Demon Monkey, claim Blade, inspect traps for Key.",
                       "A narrow run-gallery with dart slits, a Blade, and a carved niche.", &bladeItem, &howler),
          cisternRoom("cistern", "3. Sunken Cistern", "Fight Fang Demon Monkey & solve Riddle I at the gong shrine.",
                      "Flooded stone slabs and a sunken gong shrine where a Fang Demon Monkey lurks.", nullptr, &brute),
          forgeRoom("forge", "4. Obsidian Forge", "Fight Ember Demon Monkey, claim Herb, solve Riddle III at anvil.",
                    "Heat rises over dark slag, a crimson Herb, and a runed anvil.", &herbItem1, &emberAlpha),
          sanctumRoom("sanctum", "5. Ember Sanctum", "Solve Riddle II at braziers, claim Herb, defeat Alpha Idol Monkey.",
                      "Twin braziers flank a sacred altar with a Herb before the Alpha Idol Monkey.", &herbItem2, &colossus),
          gateRoom("gate", "6. Outer Temple Gate", "Unlock with Key & solve Riddle IV to escape.",
                   "Sunlight pours through towering temple doors ahead."),
          currentRoom(&cellRoom),
          activePrompt(""),
          lastAnimAction("idle"),
          difficulty(1),
          defeatsCount(0),
          escaped(false)
    {
        cellRoom.setExits(&corridorRoom, nullptr);
        corridorRoom.setExits(&cisternRoom, &cellRoom);
        cisternRoom.setExits(&forgeRoom, &corridorRoom);
        forgeRoom.setExits(&sanctumRoom, &cisternRoom);
        sanctumRoom.setExits(&gateRoom, &forgeRoom);
        gateRoom.setExits(nullptr, &sanctumRoom);
    }

    void startNewRun(int avatarId, int levelDiff, std::ostringstream& out) {
        difficulty = (levelDiff == 2) ? 2 : 1;
        player.configureAvatar(avatarId);

        cellRoom.resetItem();
        corridorRoom.resetItem();
        cisternRoom.resetItem();
        forgeRoom.resetItem();
        sanctumRoom.resetItem();
        gateRoom.resetItem();

        floorTrap.resetResolved();
        creviceSearch.resetResolved();
        cisternRiddle.resetResolved();
        brazierPuzzle.resetResolved();
        forgeRiddle.resetResolved();
        riddleDoor.resetResolved();

        double mult = (difficulty == 2) ? 1.25 : 1.0;
        howler.resetEnemy(static_cast<int>(60 * mult), static_cast<int>(19 * mult), "Screech! Another runner slips!");
        brute.resetEnemy(static_cast<int>(75 * mult), static_cast<int>(22 * mult), "Chatter-snarl! Nowhere to vault!");
        emberAlpha.resetEnemy(static_cast<int>(90 * mult), static_cast<int>(25 * mult), "Roar-shriek! The cursed pack hunts!");
        colossus.resetEnemy(static_cast<int>(115 * mult), static_cast<int>(29 * mult), "THUNDEROUS SCREECH! Kneel!");

        currentRoom = &cellRoom;
        activePrompt = "";
        lastAnimAction = "idle";
        defeatsCount = 0;
        escaped = false;

        out << "\n=== NEW RUN: " << player.getName()
            << (difficulty == 2 ? " [Lv.2 Veteran]" : " [Lv.1 Standard]") << " ===\n";
        currentRoom->describe(out, player.getHealth(), player.getMaxHealth(), player.getStamina());
    }

    void handlePlayerDefeat(Enemy* foe, std::ostringstream& out) {
        defeatsCount++;
        lastAnimAction = "defeat";
        if (foe != nullptr) {
            foe->partialHealOnLoss();
        }
        out << "\n[KNOCKED DOWN #" << defeatsCount << "] The demon monkey overwhelms you!\n";
        out << "[Recovery Trial] Retreat to the Idol Courtyard ('south' / 'retreat') to claim missed relics, use 'herb', or 'rest' to recover stamina!\n";
        currentRoom = &cellRoom;
        player.reviveAtSanctuary(out);
        currentRoom->describe(out, player.getHealth(), player.getMaxHealth(), player.getStamina());
    }

    std::string step(const std::string& rawInput) {
        std::ostringstream out;
        std::string input = trimStr(rawInput);
        std::string lower = toLowerStr(input);
        lastAnimAction = "idle";

        if (lower.rfind("start ", 0) == 0) {
            std::string rest = trimStr(lower.substr(6));
            int av = 1;
            int lv = 1;
            if (rest.find("lyra") != std::string::npos || rest.find("2") != std::string::npos) av = 2;
            if (rest.find("thorne") != std::string::npos || rest.find("3") != std::string::npos) av = 3;
            if (rest.find("veteran") != std::string::npos || rest.find("hard") != std::string::npos) lv = 2;
            startNewRun(av, lv, out);
            return out.str();
        }

        if (escaped) {
            lastAnimAction = "escape";
            out << "\n*** TEMPLE ESCAPED! Final HP: " << player.getHealth() << " | Knockdowns: " << defeatsCount << " ***\n";
            return out.str();
        }

        if (activePrompt == "trap") {
            activePrompt = "";
            floorTrap.handleInput(player, input, out);
            lastAnimAction = "vault";
            if (!player.isAlive()) {
                handlePlayerDefeat(currentRoom->getEnemy(), out);
                return out.str();
            }
            if (!creviceSearch.isResolved() && !player.getInventory().hasItem("Key")) {
                activePrompt = "crevice";
                creviceSearch.startEncounter(player, out);
                return out.str();
            }
            currentRoom->describe(out, player.getHealth(), player.getMaxHealth(), player.getStamina());
            return out.str();
        }

        if (activePrompt == "crevice") {
            activePrompt = "";
            creviceSearch.handleInput(player, input, out);
            if (!player.isAlive()) {
                handlePlayerDefeat(currentRoom->getEnemy(), out);
                return out.str();
            }
            currentRoom->describe(out, player.getHealth(), player.getMaxHealth(), player.getStamina());
            return out.str();
        }

        if (activePrompt == "riddle_bell") {
            activePrompt = "";
            cisternRiddle.handleInput(player, input, out);
            if (!player.isAlive()) {
                handlePlayerDefeat(currentRoom->getEnemy(), out);
                return out.str();
            }
            currentRoom->describe(out, player.getHealth(), player.getMaxHealth(), player.getStamina());
            return out.str();
        }

        if (activePrompt == "riddle_forge") {
            activePrompt = "";
            forgeRiddle.handleInput(player, input, out);
            if (!player.isAlive()) {
                handlePlayerDefeat(currentRoom->getEnemy(), out);
                return out.str();
            }
            currentRoom->describe(out, player.getHealth(), player.getMaxHealth(), player.getStamina());
            return out.str();
        }

        if (activePrompt == "brazier") {
            activePrompt = "";
            brazierPuzzle.handleInput(player, input, out);
            if (!player.isAlive()) {
                handlePlayerDefeat(currentRoom->getEnemy(), out);
                return out.str();
            }
            currentRoom->describe(out, player.getHealth(), player.getMaxHealth(), player.getStamina());
            return out.str();
        }

        if (activePrompt == "riddle_gate") {
            activePrompt = "";
            if (riddleDoor.handleInput(player, input, out)) {
                escaped = true;
                lastAnimAction = "escape";
                out << "\n*** VICTORY! Escaped with " << player.getHealth() << " HP (Knockdowns: " << defeatsCount << ") ***\n";
                return out.str();
            }
            if (!player.isAlive()) {
                handlePlayerDefeat(nullptr, out);
                return out.str();
            }
            currentRoom->describe(out, player.getHealth(), player.getMaxHealth(), player.getStamina());
            return out.str();
        }

        if (lower == "take") {
            Item* item = currentRoom->takeRoomItem();
            if (item != nullptr) {
                player.getInventory().addItem(item, out);
            } else {
                out << "\n[Info] No relic left in this chamber.\n";
            }
        } else if (lower.rfind("use ", 0) == 0) {
            player.useItem(trimStr(input.substr(4)), out);
        } else if (lower == "rest") {
            player.gainStamina(40);
            player.heal(15, out);
            out << "[Breath] You catch your breath (+40 STM, +15 HP).\n";
        } else if (lower == "slash" || lower == "attack") {
            Enemy* foe = currentRoom->getEnemy();
            if (foe != nullptr && foe->isAlive()) {
                lastAnimAction = "slash";
                int dmg = player.getAttackPower();
                if (currentRoom == &sanctumRoom && brazierPuzzle.isResolved()) {
                    dmg += 16;
                    out << "\n[Ember Blessing] Blazing braziers add +16 DMG!\n";
                }
                out << "\n[Slash] " << player.getName() << " lunges with a wide strike for " << dmg << " DMG!\n";
                foe->takeDamage(dmg, out);
                if (foe->isAlive()) {
                    foe->strikePlayer(player, out, false);
                } else {
                    out << "\n[Foe Down] " << foe->getName() << " is driven back!\n";
                }
            } else {
                out << "\n[Info] No demon monkey here.\n";
            }
        } else if (lower == "vault") {
            Enemy* foe = currentRoom->getEnemy();
            if (foe != nullptr && foe->isAlive()) {
                lastAnimAction = "vault";
                if (!player.spendStamina(25)) {
                    out << "\n[Low Stamina] Need 25 STM to vault! Use 'rest' or 'slash'.\n";
                    foe->strikePlayer(player, out, false);
                } else {
                    int dmg = static_cast<int>(player.getAttackPower() * 1.45) + 10;
                    out << "\n[Temple Vault] You spring off the pillar and crash down for " << dmg << " DMG! (-25 STM)\n";
                    foe->takeDamage(dmg, out);
                    if (foe->isAlive()) {
                        out << " [Agile Leap] Your mid-air vault dodges the monkey's counter!\n";
                    } else {
                        out << "\n[Foe Down] " << foe->getName() << " is crushed!\n";
                    }
                }
            } else {
                out << "\n[Info] No demon monkey to vault over here.\n";
            }
        } else if (lower == "guard") {
            Enemy* foe = currentRoom->getEnemy();
            if (foe != nullptr && foe->isAlive()) {
                lastAnimAction = "guard";
                player.gainStamina(25);
                int counterDmg = player.getAttackPower() / 2;
                out << "\n[Iron Guard] You plant your stance (+25 STM) and riposte for " << counterDmg << " DMG!\n";
                foe->takeDamage(counterDmg, out);
                if (foe->isAlive()) {
                    foe->strikePlayer(player, out, true);
                } else {
                    out << "\n[Foe Down] " << foe->getName() << " falls to your riposte!\n";
                }
            } else {
                out << "\n[Info] No demon monkey attacking right now.\n";
            }
        } else if (lower == "torch") {
            Enemy* foe = currentRoom->getEnemy();
            if (foe != nullptr && foe->isAlive()) {
                if (!player.getInventory().hasItem("Torch")) {
                    out << "\n[No Torch] Claim the Torch in Room 1 first!\n";
                } else if (!player.spendStamina(20)) {
                    out << "\n[Low Stamina] Need 20 STM for a Torch flare!\n";
                } else {
                    lastAnimAction = "torch";
                    int dmg = 26 + (player.getInventory().hasItem("Blade") ? 8 : 0);
                    out << "\n[Torch Flare] Blinding flame sears " << foe->getName() << " for " << dmg << " DMG! (-20 STM)\n";
                    foe->takeDamage(dmg, out);
                    if (foe->isAlive()) {
                        out << " [Dazed] Singed fur slows the demon's strike!\n";
                        player.takeDamage(foe->getAttackDamage() / 2, out);
                    } else {
                        out << "\n[Foe Down] " << foe->getName() << " flees the flame!\n";
                    }
                }
            } else {
                out << "\n[Info] No demon monkey to scorch here.\n";
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
                    out << "\n[Info] Gallery traps and niche already resolved.\n";
                }
            } else if (currentRoom == &cisternRoom) {
                if (!cisternRiddle.isResolved()) {
                    activePrompt = "riddle_bell";
                    cisternRiddle.startEncounter(player, out);
                    return out.str();
                } else {
                    out << "\n[Info] Gong shrine already solved.\n";
                }
            } else if (currentRoom == &forgeRoom) {
                if (!forgeRiddle.isResolved()) {
                    activePrompt = "riddle_forge";
                    forgeRiddle.startEncounter(player, out);
                    return out.str();
                } else {
                    out << "\n[Info] Anvil riddle already solved.\n";
                }
            } else if (currentRoom == &sanctumRoom) {
                if (!brazierPuzzle.isResolved()) {
                    activePrompt = "brazier";
                    brazierPuzzle.startEncounter(player, out);
                    return out.str();
                } else {
                    out << "\n[Info] Braziers already lit.\n";
                }
            } else {
                out << "\n[Info] Nothing hidden to inspect here.\n";
            }
        } else if (lower == "door") {
            if (currentRoom == &gateRoom) {
                if (!player.getInventory().hasItem("Key")) {
                    riddleDoor.startEncounter(player, out);
                } else {
                    activePrompt = "riddle_gate";
                    riddleDoor.startEncounter(player, out);
                    return out.str();
                }
            } else {
                out << "\n[Info] Reach Room 6 (Outer Temple Gate) to unlock the exit.\n";
            }
        } else if (lower == "north") {
            Enemy* foe = currentRoom->getEnemy();
            if (foe != nullptr && foe->isAlive()) {
                out << "\n[Blocked] " << foe->getName() << " blocks the path! Fight with slash, vault, guard, or torch.\n";
            } else if (currentRoom == &cisternRoom && !cisternRiddle.isResolved()) {
                out << "\n[Flooded] Inspect ('search') and solve Riddle I first.\n";
            } else if (currentRoom == &forgeRoom && !forgeRiddle.isResolved()) {
                out << "\n[Sealed] Inspect ('search') and solve Riddle III first.\n";
            } else if (currentRoom->getNorth() != nullptr) {
                currentRoom = currentRoom->getNorth();
                player.gainStamina(10);
            } else {
                out << "\n[Info] No northern path here.\n";
            }
        } else if (lower == "south" || lower == "retreat") {
            if (currentRoom->getSouth() != nullptr) {
                currentRoom = currentRoom->getSouth();
                player.gainStamina(15);
            } else {
                out << "\n[Info] Already at the southern idol courtyard.\n";
            }
        } else if (lower == "look" || lower == "help") {
            out << "\n[Moves] slash | vault | guard | torch | rest | take | search | use herb | door | north | south\n";
        } else {
            out << "\n[?] Unknown command '" << input << "'.\n";
        }

        if (!player.isAlive()) {
            handlePlayerDefeat(currentRoom->getEnemy(), out);
            return out.str();
        }

        currentRoom->describe(out, player.getHealth(), player.getMaxHealth(), player.getStamina());
        return out.str();
    }

    std::string getRoomId() const { return currentRoom->getId(); }
    int getHp() const { return player.getHealth(); }
    int getMaxHp() const { return player.getMaxHealth(); }
    int getStamina() const { return player.getStamina(); }
    int getMaxStamina() const { return player.getMaxStamina(); }
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
    const char* getPromptModeCstr() const { return activePrompt.c_str(); }
    const char* getLastAnimCstr() const { return lastAnimAction.c_str(); }
    int getEscapedFlag() const { return escaped ? 1 : 0; }
    int getAvatarId() const { return player.getAvatarId(); }
    int getDefeatsCount() const { return defeatsCount; }
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
EXPORT int current_stamina() { return g_engine.getStamina(); }
EXPORT int max_stamina() { return g_engine.getMaxStamina(); }
EXPORT int has_torch_flag() { return g_engine.getHasTorch(); }
EXPORT int has_blade_flag() { return g_engine.getHasBlade(); }
EXPORT int has_herb_flag() { return g_engine.getHasHerb(); }
EXPORT int has_key_flag() { return g_engine.getHasKey(); }
EXPORT int enemy_hp() { return g_engine.getEnemyHp(); }
EXPORT int enemy_max_hp() { return g_engine.getEnemyMaxHp(); }
EXPORT const char* enemy_name() { return g_engine.getEnemyNameCstr(); }
EXPORT const char* prompt_mode() { return g_engine.getPromptModeCstr(); }
EXPORT const char* last_anim() { return g_engine.getLastAnimCstr(); }
EXPORT int is_escaped_flag() { return g_engine.getEscapedFlag(); }
EXPORT int current_avatar_id() { return g_engine.getAvatarId(); }
EXPORT int defeats_count() { return g_engine.getDefeatsCount(); }
