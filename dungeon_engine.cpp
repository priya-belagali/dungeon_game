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
        out << "\n[Torch] You hoist the idol torch—flames lick across the chamber carvings!\n";
        return false;
    }
};

class HerbItem : public Item {
private:
    int healAmount;

public:
    HerbItem(int heal = 45)
        : Item("Herb", "Restorative elixir leaf that heals 45 HP."), healAmount(heal) {}

    int getHealAmount() const { return healAmount; }
    bool use(Player& player, std::ostringstream& out) override;
};

class BladeItem : public Item {
private:
    int bonusDamage;

public:
    BladeItem(int dmg = 22)
        : Item("Blade", "Tempered runed blade (+22 strike damage)."), bonusDamage(dmg) {}

    int getBonusDamage() const { return bonusDamage; }
    bool use(Player&, std::ostringstream& out) override {
        out << "\n[Blade] You draw your runed blade—steel gleams with latent power.\n";
        return false;
    }
};

class KeyItem : public Item {
public:
    KeyItem() : Item("Key", "Ancient seal key that opens locked vaults and gates.") {}
    bool use(Player&, std::ostringstream& out) override {
        out << "\n[Key] The seal key hums with resonant runes.\n";
        return false;
    }
};

class RelicItem : public Item {
public:
    RelicItem() : Item("Relic", "The Astral World-Shattering Relic—pulsing with apocalyptic magic.") {}
    bool use(Player&, std::ostringstream& out) override {
        out << "\n[Relic] Dark cosmic energy surges through the Astral Relic in your grasp!\n";
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
    Player() : Entity("Kaelen", 140), baseAttack(24), avatarId(1), stamina(100), maxStamina(100) {}

    void configureAvatar(int id) {
        avatarId = id;
        inventory.reset();
        stamina = 100;
        maxStamina = 100;
        if (id == 2) {
            setStats("Lyra", 125);
            baseAttack = 30;
        } else if (id == 3) {
            setStats("Thorne", 160);
            baseAttack = 22;
        } else {
            avatarId = 1;
            setStats("Kaelen", 140);
            baseAttack = 24;
        }
    }

    void reviveAtSanctuary(std::ostringstream& out) {
        health = static_cast<int>(maxHealth * 0.70);
        if (health < 50) health = 50;
        stamina = maxStamina;
        out << "\n[Regroup] You rally at the starting threshold and recover "
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
        if (inventory.hasItem("Relic")) total += 24;
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
// 3. POLYMORPHISM: Diverse Villains & Obstacles
// ==========================================
class Enemy : public Entity {
protected:
    int attackDamage;
    std::string taunt;
    std::string villainKind; // "monkey", "wraith", "golem", "seraph"

public:
    Enemy(std::string n, std::string kind, int hp, int dmg, std::string t)
        : Entity(n, hp), attackDamage(dmg), taunt(t), villainKind(kind) {}

    const std::string& getKind() const { return villainKind; }

    void resetEnemy(int hp, int dmg, std::string t) {
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
            out << " [Guard] Your stance cuts the enemy's strike in half!\n";
        }
        player.takeDamage(dmg, out);
    }
};

class DemonMonkeyVillain : public Enemy {
public:
    DemonMonkeyVillain()
        : Enemy("Demon Monkey", "monkey", 62, 18, "Screech! Another trespasser enters the ruins!") {}
};

class ShadowWraithVillain : public Enemy {
public:
    ShadowWraithVillain()
        : Enemy("Shadow Wraith", "wraith", 76, 21, "Your soul will linger in the mist!") {}
};

class IronGolemVillain : public Enemy {
public:
    IronGolemVillain()
        : Enemy("Obsidian Golem", "golem", 92, 24, "Stone and fury guard this relic!") {}
};

class AstralSentinelVillain : public Enemy {
public:
    AstralSentinelVillain()
        : Enemy("Astral Warden", "seraph", 110, 27, "You cannot wield the Relic to end the world!") {}
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
            player.takeDamage(14, out);
        }
        out << "\n[Hazard] Floor spikes snap ahead while darts hiss! Choose move: slide, leap, or brace.\n";
    }

    bool handleInput(Player& player, const std::string& input, std::ostringstream& out) override {
        std::string c = toLowerStr(trimStr(input));
        if (c == "slide" || c == "dodge" || c == "leap") {
            out << "\n[Clean Move] You clear the trap cleanly!\n";
        } else if (c == "brace" || c == "block") {
            out << "\n[Glancing Hit] You brace and take minor damage.\n";
            player.takeDamage(8, out);
        } else {
            out << "\n[Stumble] Late footwork catches a dart!\n";
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
    CreviceSearch(KeyItem* key) : Obstacle("Carved Niche"), keyRef(key) {}

    void startEncounter(Player&, std::ostringstream& out) override {
        out << "\n[Niche] A seal key glints inside a split stone wall. Reach left or right?\n";
    }

    bool handleInput(Player& player, const std::string& input, std::ostringstream& out) override {
        std::string c = toLowerStr(trimStr(input));
        if (c == "right") {
            out << "\n[Claimed] You reach right and claim the Key!\n";
            player.getInventory().addItem(keyRef, out);
            resolved = true;
            return true;
        }
        out << "\n[Ambush] A hidden snare snaps from that side!\n";
        player.takeDamage(14, out);
        return false;
    }
};

class CisternRiddle : public Obstacle {
public:
    CisternRiddle() : Obstacle("Flooded Gong Shrine") {}

    void startEncounter(Player&, std::ostringstream& out) override {
        out << "\n[Riddle I] Runes glow over the water:\n";
        out << "\"I speak without a mouth and hear without ears. I have no body, but I come alive with wind. What am I?\"\n";
    }

    bool handleInput(Player& player, const std::string& input, std::ostringstream& out) override {
        std::string c = toLowerStr(trimStr(input));
        if (c == "echo" || c == "an echo") {
            out << "\n[Solved] Stepping stones rise above the water!\n";
            resolved = true;
            return true;
        }
        out << "\n[Wrong] Freezing spray lashes across the hall!\n";
        player.takeDamage(15, out);
        return false;
    }
};

class BrazierPuzzle : public Obstacle {
public:
    BrazierPuzzle() : Obstacle("Twin Flame Braziers") {}

    void startEncounter(Player&, std::ostringstream& out) override {
        out << "\n[Riddle II] Carved runes read:\n";
        out << "\"I consume forests, drink rivers, and fear no blade—yet a single raindrop can end me. What am I?\"\n";
    }

    bool handleInput(Player& player, const std::string& input, std::ostringstream& out) override {
        std::string c = toLowerStr(trimStr(input));
        if (c == "fire" || c == "flame") {
            out << "\n[Solved] Flame surges across the braziers!\n";
            resolved = true;
            return true;
        }
        out << "\n[Wrong] Cold ash bursts from the braziers!\n";
        player.takeDamage(15, out);
        return false;
    }
};

class ForgeRiddle : public Obstacle {
public:
    ForgeRiddle() : Obstacle("Obsidian Anvil Seal") {}

    void startEncounter(Player&, std::ostringstream& out) override {
        out << "\n[Riddle III] Glowing runes pulse across the anvil:\n";
        out << "\"The more you take from me, the bigger I grow. What am I?\"\n";
    }

    bool handleInput(Player& player, const std::string& input, std::ostringstream& out) override {
        std::string c = toLowerStr(trimStr(input));
        if (c == "hole" || c == "a hole") {
            out << "\n[Solved] The seal cracks open!\n";
            resolved = true;
            return true;
        }
        out << "\n[Wrong] Molten cinders spurt from the anvil!\n";
        player.takeDamage(16, out);
        return false;
    }
};

class RiddleDoor : public Obstacle {
public:
    RiddleDoor() : Obstacle("Final Gate") {}

    void startEncounter(Player& player, std::ostringstream& out) override {
        if (!player.getInventory().hasItem("Key")) {
            out << "\n[Sealed] You need the [Key] to unlock this gate.\n";
            return;
        }
        out << "\n[Final Riddle] The ancient lock asks:\n";
        out << "\"What gets wetter the more it dries?\"\n";
    }

    bool handleInput(Player& player, const std::string& input, std::ostringstream& out) override {
        std::string c = toLowerStr(trimStr(input));
        if (c == "towel" || c == "a towel") {
            out << "\n[UNLOCKED] The final seal shatters open!\n";
            resolved = true;
            return true;
        }
        out << "\n[Wrong] Shock-runes flash across the lock!\n";
        player.takeDamage(18, out);
        return false;
    }
};

// ==========================================
// 4. ROOM GRAPH & MULTI-LEVEL ENGINE
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

    void setRoomData(std::string t, std::string hint, std::string desc, Item* item, Enemy* roomEnemy) {
        title = t;
        objectiveHint = hint;
        description = desc;
        roomItem = item;
        initialItem = item;
        enemy = roomEnemy;
    }

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
            out << " * Item: [" << roomItem->getName() << "] ('take')\n";
        }
        if (enemy != nullptr && enemy->isAlive()) {
            out << " * Villain: " << enemy->getName() << " (" << enemy->getHealth()
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
    RelicItem astralRelic;

    DemonMonkeyVillain monkeyEnemy;
    ShadowWraithVillain wraithEnemy;
    IronGolemVillain golemEnemy;
    AstralSentinelVillain sentinelEnemy;

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
    int levelNumber; // 1, 2, or 3
    int defeatsCount;
    bool worldDestroyed;

public:
    GameEngine()
        : player(),
          torchItem(),
          bladeItem(22),
          herbItem1(45),
          herbItem2(45),
          keyItem(),
          astralRelic(),
          monkeyEnemy(),
          wraithEnemy(),
          golemEnemy(),
          sentinelEnemy(),
          floorTrap(),
          creviceSearch(&keyItem),
          cisternRiddle(),
          brazierPuzzle(),
          forgeRiddle(),
          riddleDoor(),
          cellRoom("cell", "1. Ruined Courtyard", "", "", &torchItem, nullptr),
          corridorRoom("corridor", "2. Trapped Gallery", "", "", &bladeItem, &monkeyEnemy),
          cisternRoom("cistern", "3. Sunken Cistern", "", "", nullptr, &wraithEnemy),
          forgeRoom("forge", "4. Obsidian Forge", "", "", &herbItem1, &golemEnemy),
          sanctumRoom("sanctum", "5. Inner Sanctum", "", "", &herbItem2, &sentinelEnemy),
          gateRoom("gate", "6. Final Gate", "", ""),
          currentRoom(&cellRoom),
          activePrompt(""),
          lastAnimAction("idle"),
          levelNumber(1),
          defeatsCount(0),
          worldDestroyed(false)
    {
        cellRoom.setExits(&corridorRoom, nullptr);
        corridorRoom.setExits(&cisternRoom, &cellRoom);
        cisternRoom.setExits(&forgeRoom, &corridorRoom);
        forgeRoom.setExits(&sanctumRoom, &cisternRoom);
        sanctumRoom.setExits(&gateRoom, &forgeRoom);
        gateRoom.setExits(nullptr, &sanctumRoom);
        configureLevel(1, true, nullptr);
    }

    void configureLevel(int lvl, bool resetPlayer, std::ostringstream* outPtr) {
        levelNumber = (lvl < 1) ? 1 : ((lvl > 3) ? 3 : lvl);
        if (resetPlayer) {
            player.configureAvatar(player.getAvatarId());
            defeatsCount = 0;
        } else {
            player.gainStamina(100);
            player.heal(40, *outPtr);
        }

        floorTrap.resetResolved();
        creviceSearch.resetResolved();
        cisternRiddle.resetResolved();
        brazierPuzzle.resetResolved();
        forgeRiddle.resetResolved();
        riddleDoor.resetResolved();
        activePrompt = "";
        lastAnimAction = "idle";
        worldDestroyed = false;
        currentRoom = &cellRoom;

        if (levelNumber == 1) {
            monkeyEnemy.resetEnemy(62, 18, "Screech! Another runner slips on the stones!");
            wraithEnemy.resetEnemy(74, 20, "Cold mist clings to your stride!");
            golemEnemy.resetEnemy(88, 23, "Stone fists slam the temple floor!");
            sentinelEnemy.resetEnemy(104, 26, "The temple wards will not yield!");

            cellRoom.setRoomData("1. Sunlit Courtyard", "Claim Torch, then run North.",
                                 "Overgrown temple steps with a flickering Torch.", &torchItem, nullptr);
            corridorRoom.setRoomData("2. Trapped Gallery", "Defeat Demon Monkey, claim Blade, inspect for Key.",
                                     "Dart-slit hall guarded by a Demon Monkey.", &bladeItem, &monkeyEnemy);
            cisternRoom.setRoomData("3. Sunken Cistern", "Defeat Shadow Wraith & inspect for Riddle I.",
                                    "Flooded basin where a Shadow Wraith drifts.", nullptr, &wraithEnemy);
            forgeRoom.setRoomData("4. Ember Forge", "Defeat Obsidian Golem, claim Herb, inspect for Riddle III.",
                                  "Glowing furnace chamber with an Obsidian Golem.", &herbItem1, &golemEnemy);
            sanctumRoom.setRoomData("5. Ward Sanctum", "Inspect braziers (Riddle II) & defeat Astral Warden.",
                                    "Altar hall guarded by the Astral Warden.", &herbItem2, &sentinelEnemy);
            gateRoom.setRoomData("6. Level 1 Exit Gate", "Use Key & solve Riddle to advance to Level 2.",
                                 "Ancient archway leading deeper toward the Relic.", nullptr, nullptr);
        } else if (levelNumber == 2) {
            monkeyEnemy.resetEnemy(70, 20, "The Relic's guardians strike fast!");
            wraithEnemy.resetEnemy(84, 23, "Shadows bow before the Ancient Relic!");
            golemEnemy.resetEnemy(98, 25, "None claim the Relic without breaking me!");
            sentinelEnemy.resetEnemy(118, 28, "Stop! That Relic carries catastrophic power!");

            cellRoom.setRoomData("1. Relic Approach", "Run North toward the Ancient Relic Vault.",
                                 "Carved causeway continuing from Level 1 on the hunt for the magical Relic.", nullptr, nullptr);
            corridorRoom.setRoomData("2. Whispering Arch", "Defeat Demon Monkey & inspect for Key.",
                                     "A Demon Monkey ambushes Relic seekers.", nullptr, &monkeyEnemy);
            cisternRoom.setRoomData("3. Mirror Basin", "Defeat Shadow Wraith & solve Riddle I.",
                                    "Dark reflecting water haunted by a Shadow Wraith.", nullptr, &wraithEnemy);
            forgeRoom.setRoomData("4. Relic Pedestal", "Claim the magical [Relic] ('take') & defeat Obsidian Golem.",
                                  "Upon a floating altar rests the Ancient Relic humming with dark magic!", &astralRelic, &golemEnemy);
            sanctumRoom.setRoomData("5. Shattered Shrine", "Claim Herb & defeat Astral Warden guarding the exit.",
                                    "The Astral Warden senses the Relic's dark pulse!", &herbItem1, &sentinelEnemy);
            gateRoom.setRoomData("6. Level 2 Gate", "Unlock with Key & solve Riddle to enter Level 3.",
                                 "Beyond this gate lies the final showdown for the world.", nullptr, nullptr);
        } else {
            // Level 3: Plot twist — hero turns out to be the villain aiming to destroy the world with the Relic!
            if (!player.getInventory().hasItem("Relic")) {
                std::ostringstream dummy;
                player.getInventory().addItem(&astralRelic, dummy);
            }
            monkeyEnemy.resetEnemy(78, 22, "Traitor! You want to shatter the world!");
            wraithEnemy.resetEnemy(92, 24, "The Relic's dark curse fuels your betrayal!");
            golemEnemy.resetEnemy(108, 27, "I will crush your world-ending plan!");
            sentinelEnemy.resetEnemy(128, 30, "End this madness before the world falls!");

            cellRoom.setRoomData("1. Betrayal Threshold", "Run North—unleash the Relic to destroy the world!",
                                 "TWIST: You are the true villain—using the Level 2 Relic to destroy the world!", nullptr, nullptr);
            corridorRoom.setRoomData("2. Cursed Bridge", "Defeat Demon Monkey & inspect for Key.",
                                     "A Demon Monkey tries to block your dark crusade.", nullptr, &monkeyEnemy);
            cisternRoom.setRoomData("3. Abyssal Well", "Defeat Shadow Wraith & solve Riddle I.",
                                    "A Shadow Wraith rises against your Relic's power.", nullptr, &wraithEnemy);
            forgeRoom.setRoomData("4. Doom Crucible", "Claim Herb, solve Riddle III & push North.",
                                  "The Relic amplifies every strike as you march toward apocalypse.", &herbItem2, &golemEnemy);
            sanctumRoom.setRoomData("5. Throne of Ruin", "Defeat the Astral Warden blocking the Final Gate.",
                                    "The final guardian stands between you and total world destruction.", nullptr, &sentinelEnemy);
            gateRoom.setRoomData("6. World-Shattering Altar", "Use Key & solve Final Riddle to DESTROY THE WORLD!",
                                 "Activate the Relic at the Final Gate to win and destroy the world!", nullptr, nullptr);
        }

        if (outPtr != nullptr) {
            *outPtr << "\n=== LEVEL " << levelNumber << " ===\n";
            currentRoom->describe(*outPtr, player.getHealth(), player.getMaxHealth(), player.getStamina());
        }
    }

    void startNewRun(int avatarId, int startLvl, std::ostringstream& out) {
        player.configureAvatar(avatarId);
        configureLevel(startLvl, true, &out);
    }

    void handlePlayerDefeat(Enemy* foe, std::ostringstream& out) {
        defeatsCount++;
        lastAnimAction = "defeat";
        if (foe != nullptr) {
            foe->partialHealOnLoss();
        }
        out << "\n[HURT / KNOCKED DOWN #" << defeatsCount << "] You took heavy damage!\n";
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
            if (rest.find("level2") != std::string::npos || rest.find("lvl2") != std::string::npos) lv = 2;
            if (rest.find("level3") != std::string::npos || rest.find("lvl3") != std::string::npos) lv = 3;
            startNewRun(av, lv, out);
            return out.str();
        }

        if (lower == "timeout") {
            out << "\n[TIME EXPIRED] The countdown ran out before you unleashed the Relic!\n";
            handlePlayerDefeat(currentRoom->getEnemy(), out);
            return out.str();
        }

        if (worldDestroyed) {
            lastAnimAction = "apocalypse";
            out << "\n*** VICTORY! THE WORLD IS DESTROYED BY THE RELIC! ***\n";
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
                if (levelNumber == 1) {
                    out << "\n[LEVEL 1 COMPLETE] Advancing to Level 2 to claim the Ancient Relic!\n";
                    configureLevel(2, false, &out);
                    lastAnimAction = "level_up";
                    return out.str();
                } else if (levelNumber == 2) {
                    if (!player.getInventory().hasItem("Relic")) {
                        player.getInventory().addItem(&astralRelic, out);
                    }
                    out << "\n[LEVEL 2 COMPLETE] You hold the Relic! Entering Level 3—where your true villainous goal awakens!\n";
                    configureLevel(3, false, &out);
                    lastAnimAction = "twist";
                    return out.str();
                } else {
                    worldDestroyed = true;
                    lastAnimAction = "apocalypse";
                    out << "\n*** THE RELIC SHATTERS THE SKY—THE WORLD IS DESTROYED! YOU WIN! ***\n";
                    return out.str();
                }
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
                lastAnimAction = (item->getName() == "Relic") ? "relic_found" : "take";
            } else {
                out << "\n[Info] Nothing left to take here.\n";
            }
        } else if (lower.rfind("use ", 0) == 0) {
            player.useItem(trimStr(input.substr(4)), out);
        } else if (lower == "rest") {
            player.gainStamina(40);
            player.heal(15, out);
            out << "\n[Rest] You catch your breath (+40 STM, +15 HP).\n";
        } else if (lower == "slash" || lower == "attack") {
            Enemy* foe = currentRoom->getEnemy();
            if (foe != nullptr && foe->isAlive()) {
                lastAnimAction = "slash";
                int dmg = player.getAttackPower();
                out << "\n[Hero Attack] " << player.getName() << " strikes " << foe->getName() << " for " << dmg << " DMG!\n";
                foe->takeDamage(dmg, out);
                if (foe->isAlive()) {
                    foe->strikePlayer(player, out, false);
                } else {
                    out << "\n[Defeated] " << foe->getName() << " falls!\n";
                }
            } else {
                out << "\n[Info] No villain here to fight.\n";
            }
        } else if (lower == "vault") {
            Enemy* foe = currentRoom->getEnemy();
            if (foe != nullptr && foe->isAlive()) {
                lastAnimAction = "vault";
                if (!player.spendStamina(25)) {
                    out << "\n[Low Stamina] Need 25 STM to vault!\n";
                    foe->strikePlayer(player, out, false);
                } else {
                    int dmg = static_cast<int>(player.getAttackPower() * 1.45) + 10;
                    out << "\n[Vault Strike] You leap and slam " << foe->getName() << " for " << dmg << " DMG! (-25 STM)\n";
                    foe->takeDamage(dmg, out);
                    if (foe->isAlive()) {
                        out << " [Evade] You vault clear of the counterattack!\n";
                    } else {
                        out << "\n[Defeated] " << foe->getName() << " is crushed!\n";
                    }
                }
            } else {
                out << "\n[Info] No villain here.\n";
            }
        } else if (lower == "guard") {
            Enemy* foe = currentRoom->getEnemy();
            if (foe != nullptr && foe->isAlive()) {
                lastAnimAction = "guard";
                player.gainStamina(25);
                int counterDmg = player.getAttackPower() / 2;
                out << "\n[Guard Riposte] You guard (+25 STM) and counter for " << counterDmg << " DMG!\n";
                foe->takeDamage(counterDmg, out);
                if (foe->isAlive()) {
                    foe->strikePlayer(player, out, true);
                } else {
                    out << "\n[Defeated] " << foe->getName() << " falls to your riposte!\n";
                }
            } else {
                out << "\n[Info] No villain attacking here.\n";
            }
        } else if (lower == "torch") {
            Enemy* foe = currentRoom->getEnemy();
            if (foe != nullptr && foe->isAlive()) {
                if (!player.getInventory().hasItem("Torch")) {
                    out << "\n[No Torch] Claim the Torch first!\n";
                } else if (!player.spendStamina(20)) {
                    out << "\n[Low Stamina] Need 20 STM for Torch Flare!\n";
                } else {
                    lastAnimAction = "torch";
                    int dmg = 26 + (player.getInventory().hasItem("Blade") ? 8 : 0);
                    out << "\n[Torch Flare] Blazing flame sears " << foe->getName() << " for " << dmg << " DMG! (-20 STM)\n";
                    foe->takeDamage(dmg, out);
                    if (foe->isAlive()) {
                        player.takeDamage(foe->getAttackDamage() / 2, out);
                    } else {
                        out << "\n[Defeated] " << foe->getName() << " is driven back!\n";
                    }
                }
            } else {
                out << "\n[Info] No villain here.\n";
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
                    out << "\n[Info] Already inspected.\n";
                }
            } else if (currentRoom == &cisternRoom) {
                if (!cisternRiddle.isResolved()) {
                    activePrompt = "riddle_bell";
                    cisternRiddle.startEncounter(player, out);
                    return out.str();
                } else {
                    out << "\n[Info] Already solved.\n";
                }
            } else if (currentRoom == &forgeRoom) {
                if (!forgeRiddle.isResolved()) {
                    activePrompt = "riddle_forge";
                    forgeRiddle.startEncounter(player, out);
                    return out.str();
                } else {
                    out << "\n[Info] Already solved.\n";
                }
            } else if (currentRoom == &sanctumRoom) {
                if (!brazierPuzzle.isResolved()) {
                    activePrompt = "brazier";
                    brazierPuzzle.startEncounter(player, out);
                    return out.str();
                } else {
                    out << "\n[Info] Already lit.\n";
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
                out << "\n[Info] Reach Room 6 (Final Gate) to use the gate.\n";
            }
        } else if (lower == "north") {
            Enemy* foe = currentRoom->getEnemy();
            if (foe != nullptr && foe->isAlive()) {
                out << "\n[Blocked] " << foe->getName() << " blocks the path! Fight first.\n";
            } else if (currentRoom == &cisternRoom && !cisternRiddle.isResolved()) {
                out << "\n[Sealed] Inspect ('search') and solve Riddle I first.\n";
            } else if (currentRoom == &forgeRoom && !forgeRiddle.isResolved()) {
                out << "\n[Sealed] Inspect ('search') and solve Riddle III first.\n";
            } else if (currentRoom->getNorth() != nullptr) {
                lastAnimAction = "run_north";
                currentRoom = currentRoom->getNorth();
                player.gainStamina(10);
            } else {
                out << "\n[Info] No northern path here.\n";
            }
        } else if (lower == "south" || lower == "retreat") {
            if (currentRoom->getSouth() != nullptr) {
                lastAnimAction = "run_south";
                currentRoom = currentRoom->getSouth();
                player.gainStamina(15);
            } else {
                out << "\n[Info] Already at the southern start.\n";
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
    int getHasRelic() const { return player.getInventory().hasItem("Relic") ? 1 : 0; }
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
    const char* getEnemyKindCstr() const {
        static std::string ekind;
        Enemy* foe = currentRoom->getEnemy();
        ekind = (foe != nullptr && foe->isAlive()) ? foe->getKind() : "";
        return ekind.c_str();
    }
    const char* getPromptModeCstr() const { return activePrompt.c_str(); }
    const char* getLastAnimCstr() const { return lastAnimAction.c_str(); }
    int getLevelNumber() const { return levelNumber; }
    int getWorldDestroyedFlag() const { return worldDestroyed ? 1 : 0; }
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
EXPORT int has_relic_flag() { return g_engine.getHasRelic(); }
EXPORT int enemy_hp() { return g_engine.getEnemyHp(); }
EXPORT int enemy_max_hp() { return g_engine.getEnemyMaxHp(); }
EXPORT const char* enemy_name() { return g_engine.getEnemyNameCstr(); }
EXPORT const char* enemy_kind() { return g_engine.getEnemyKindCstr(); }
EXPORT const char* prompt_mode() { return g_engine.getPromptModeCstr(); }
EXPORT const char* last_anim() { return g_engine.getLastAnimCstr(); }
EXPORT int current_level() { return g_engine.getLevelNumber(); }
EXPORT int is_world_destroyed() { return g_engine.getWorldDestroyedFlag(); }
EXPORT int is_escaped_flag() { return g_engine.getWorldDestroyedFlag(); }
EXPORT int current_avatar_id() { return g_engine.getAvatarId(); }
EXPORT int defeats_count() { return g_engine.getDefeatsCount(); }
