#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cctype>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#define EXPORT extern "C" EMSCRIPTEN_KEEPALIVE
#else
#define EXPORT extern "C"
#endif

static std::string trimStr(const std::string& s) {
    size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) ++start;
    size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) --end;
    return s.substr(start, end - start);
}

static std::string toLowerStr(const std::string& s) {
    std::string out = s;
    for (char& c : out) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return out;
}

class Player;

class Item {
protected:
    std::string name;
    std::string description;
public:
    Item(std::string n, std::string desc) : name(n), description(desc) {}
    virtual ~Item() = default;
    std::string getName() const { return name; }
    std::string getDescription() const { return description; }
    virtual bool use(Player& player, std::ostringstream& out) = 0;
};

class TorchItem : public Item {
public:
    TorchItem() : Item("Torch", "A warm flame that cuts shadows and steadies nerves (+30 STM on use).") {}
    bool use(Player& player, std::ostringstream& out) override;
};

class HerbItem : public Item {
private:
    int healAmount;
public:
    HerbItem(int amt = 45) : Item("Herb", "Crushed leaves that mend wounds (+45 HP on use)."), healAmount(amt) {}
    bool use(Player& player, std::ostringstream& out) override;
};

class BladeItem : public Item {
public:
    BladeItem() : Item("Blade", "A forged edge that sharpens every strike.") {}
    bool use(Player&, std::ostringstream& out) override {
        out << "\n[Blade] Drawn and balanced in hand.\n";
        return false;
    }
};

class KeyItem : public Item {
public:
    KeyItem() : Item("Key", "An iron-and-brass ward meant for heavy locks.") {}
    bool use(Player&, std::ostringstream& out) override {
        out << "\n[Key] It hums faintly near ancient gates.\n";
        return false;
    }
};

class RelicItem : public Item {
public:
    RelicItem() : Item("Relic", "An engraved shard resonating with the chamber seals.") {}
    bool use(Player&, std::ostringstream& out) override {
        out << "\n[Relic] Warmth pulses from its etched rings.\n";
        return false;
    }
};

class Inventory {
private:
    std::vector<Item*> items;
public:
    void reset() { items.clear(); }
    bool hasItem(const std::string& itemName) const {
        std::string target = toLowerStr(itemName);
        for (Item* it : items) {
            if (toLowerStr(it->getName()) == target) return true;
        }
        return false;
    }
    Item* getItem(const std::string& itemName) {
        std::string target = toLowerStr(itemName);
        for (Item* it : items) {
            if (toLowerStr(it->getName()) == target) return it;
        }
        return nullptr;
    }
    void addItem(Item* item, std::ostringstream& out) {
        if (!item) return;
        if (hasItem(item->getName())) {
            out << "\n[Pack] You already carry [" << item->getName() << "].\n";
            return;
        }
        items.push_back(item);
        out << "\n[Taken] [" << item->getName() << "] — " << item->getDescription() << "\n";
    }
    void removeItem(const std::string& itemName) {
        std::string target = toLowerStr(itemName);
        items.erase(std::remove_if(items.begin(), items.end(),
            [&](Item* it) { return toLowerStr(it->getName()) == target; }), items.end());
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
    void setStats(std::string n, int hp) { name = n; health = hp; maxHealth = hp; }
    virtual void takeDamage(int amount, std::ostringstream& out) {
        health = std::max(0, health - amount);
        out << " [!] " << name << " takes " << amount << " DMG (" << health << "/" << maxHealth << " HP)\n";
    }
    virtual void heal(int amount, std::ostringstream& out) {
        health = std::min(maxHealth, health + amount);
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

    int getAvatarId() const { return avatarId; }
    int getStamina() const { return stamina; }
    int getMaxStamina() const { return maxStamina; }

    bool spendStamina(int cost) {
        if (stamina < cost) return false;
        stamina -= cost;
        return true;
    }

    void drainMoveStamina(int cost, std::ostringstream& out) {
        stamina = std::max(0, stamina - cost);
        if (stamina == 0) {
            out << " [Exhausted] Out of stamina! -10 HP.\n";
            takeDamage(10, out);
        }
    }

    void gainStamina(int amount) {
        stamina = std::min(maxStamina, stamina + amount);
    }

    Inventory& getInventory() { return inventory; }
    const Inventory& getInventory() const { return inventory; }

    int getAttackPower(int levelNumber) const {
        int total = baseAttack + (levelNumber - 1) * 5;
        if (inventory.hasItem("Blade")) total += 18 + levelNumber * 4;
        if (inventory.hasItem("Torch")) total += 6;
        if (inventory.hasItem("Relic")) total += (levelNumber == 3 ? 30 : 18);
        return total;
    }

    void useItem(const std::string& itemName, std::ostringstream& out) {
        Item* item = inventory.getItem(itemName);
        if (!item) {
            out << "\n[!] No '" << itemName << "' in pack.\n";
            return;
        }
        if (item->use(*this, out)) {
            inventory.removeItem(itemName);
        }
    }
};

bool TorchItem::use(Player& player, std::ostringstream& out) {
    player.gainStamina(30);
    out << "\n[Torch] Flame steadies your breath (+30 STM).\n";
    return false;
}

bool HerbItem::use(Player& player, std::ostringstream& out) {
    player.heal(healAmount, out);
    return true;
}

class Enemy : public Entity {
protected:
    int attackDamage;
    std::string taunt;
    std::string villainKind;
public:
    Enemy(std::string n, std::string kind, int hp, int dmg, std::string t)
        : Entity(n, hp), attackDamage(dmg), taunt(t), villainKind(kind) {}
    const std::string& getKind() const { return villainKind; }
    int getAttackDamage() const { return attackDamage; }
    void resetEnemy(int hp, int dmg, std::string t) {
        health = hp; maxHealth = hp; attackDamage = dmg; taunt = t;
    }
    virtual void strikePlayer(Player& player, std::ostringstream& out, bool guarded = false) {
        out << " [" << name << "] \"" << taunt << "\"\n";
        int dmg = guarded ? std::max(6, attackDamage / 2) : attackDamage;
        if (guarded) out << " [Guard] Strike blunted!\n";
        player.takeDamage(dmg, out);
    }
};

class DemonMonkeyVillain : public Enemy {
public:
    DemonMonkeyVillain() : Enemy("Demon Monkey", "monkey", 64, 18, "Screech! Choose your turn wisely!") {}
};
class ShadowWraithVillain : public Enemy {
public:
    ShadowWraithVillain() : Enemy("Shadow Wraith", "wraith", 78, 22, "Cold whispers linger where heavy feet tread.") {}
};
class IronGolemVillain : public Enemy {
public:
    IronGolemVillain() : Enemy("Obsidian Golem", "golem", 94, 25, "Stone remembers every seal.") {}
};
class AstralSentinelVillain : public Enemy {
public:
    AstralSentinelVillain() : Enemy("Astral Warden", "seraph", 112, 28, "Only those prepared may pass the gate.") {}
};

class Room {
private:
    std::string id;
    int chamberNum;
    std::string title;
    std::string hint;
    std::string description;
    Item* roomItem;
    Enemy* enemy;
    Room* northRoom;
    Room* southRoom;
    Room* eastRoom;
    Room* westRoom;
    bool lethalWrongTurn;
    std::string lethalMsg;
public:
    Room(std::string roomId, int num)
        : id(roomId), chamberNum(num), roomItem(nullptr), enemy(nullptr),
          northRoom(nullptr), southRoom(nullptr), eastRoom(nullptr), westRoom(nullptr),
          lethalWrongTurn(false) {}

    const std::string& getId() const { return id; }
    int getChamberNum() const { return chamberNum; }
    const std::string& getTitle() const { return title; }
    const std::string& getHint() const { return hint; }
    bool isLethal() const { return lethalWrongTurn; }
    const std::string& getLethalMsg() const { return lethalMsg; }

    void setRoomData(std::string t, std::string h, std::string desc, Item* item, Enemy* foe,
                     bool lethal = false, std::string lMsg = "") {
        title = t; hint = h; description = desc;
        roomItem = item; enemy = foe;
        lethalWrongTurn = lethal; lethalMsg = lMsg;
    }

    void setExits(Room* n, Room* s, Room* e, Room* w) {
        northRoom = n; southRoom = s; eastRoom = e; westRoom = w;
    }

    Room* getNorth() const { return northRoom; }
    Room* getSouth() const { return southRoom; }
    Room* getEast() const { return eastRoom; }
    Room* getWest() const { return westRoom; }
    Enemy* getEnemy() const { return enemy; }

    Item* takeRoomItem() {
        Item* temp = roomItem;
        roomItem = nullptr;
        return temp;
    }

    void describe(std::ostringstream& out, int hp, int maxHp, int stm) const {
        out << "\n[" << title << "] HP " << hp << "/" << maxHp << " | STM " << stm << "\n";
        out << description << "\n";
        out << " Hint: " << hint << "\n";
        if (roomItem) out << " * Found: [" << roomItem->getName() << "] ('take')\n";
        if (enemy && enemy->isAlive()) {
            out << " * Guardian: " << enemy->getName() << " (" << enemy->getHealth() << "/" << enemy->getMaxHealth() << " HP)\n";
        }
    }
};

class GameEngine {
private:
    Player player;
    TorchItem torchItem;
    BladeItem bladeItem;
    HerbItem herbItem;
    KeyItem keyItem;
    RelicItem relicItem;

    DemonMonkeyVillain monkeyEnemy;
    ShadowWraithVillain wraithEnemy;
    IronGolemVillain golemEnemy;
    AstralSentinelVillain sentinelEnemy;

    // 6-Chamber forward-progressing maze with meaningful branching:
    // Chamber 1 (cell): Courtyard [Torch] -> East to Chamber 2 (east_wing) or North to Chamber 3 (corridor)
    // Chamber 2 (east_wing): East Wing [Blade + Demon Monkey] -> East to Chamber 3 (corridor)
    // Chamber 3 (corridor): Lantern Gallery [Shadow Wraith] -> North to Chamber 4 (forge)
    // Chamber 4 (forge): Deep Hall [Relic + Obsidian Golem] -> North to Chamber 5 (sanctum) or East to Chamber 5.5/side shrine (herb_shrine)
    // Chamber 5 (sanctum): Inner Sanctum [Key + Obsidian Golem] -> North to Chamber 6 (gate)
    // Chamber 6 (gate): Final Gate [Astral Warden] -> 'door' advances/wins (requires Key)
    // Rule: Moving to any room with chamberNum < currentRoom.chamberNum is strictly blocked.
    Room r1, r2, r3, r4, r5, r6;
    Room* currentRoom;
    int levelNumber;
    int defeatsCount;
    bool worldDestroyed;
    std::string lastAnimAction;

    void wireMaze() {
        // Every chamber connects North, South, East, and West so the character can move in any direction
        r1.setExits(&r3, &r5, &r2, &r4);
        r2.setExits(&r4, &r1, &r3, &r1);
        r3.setExits(&r4, &r1, &r2, &r2);
        r4.setExits(&r5, &r3, &r5, &r2);
        r5.setExits(&r6, &r4, &r4, &r1);
        r6.setExits(&r1, &r5, &r5, &r4);
    }

    void restartCurrentLevel(const std::string& reason, std::ostringstream& out) {
        defeatsCount++;
        lastAnimAction = "defeat";
        out << "\n[Restarted] " << reason << "\n";
        configureLevel(levelNumber, out);
    }

public:
    GameEngine()
        : herbItem(45),
          r1("cell", 1), r2("east_wing", 2), r3("corridor", 3),
          r4("forge", 4), r5("sanctum", 5), r6("gate", 6),
          currentRoom(&r1), levelNumber(1), defeatsCount(0),
          worldDestroyed(false), lastAnimAction("idle") {
        wireMaze();
    }

    void configureLevel(int lvl, std::ostringstream& out) {
        levelNumber = std::max(1, std::min(3, lvl));
        player.configureAvatar(player.getAvatarId());
        worldDestroyed = false;
        lastAnimAction = "idle";
        currentRoom = &r1;

        int scale = (levelNumber - 1) * 18;
        int dmgScale = (levelNumber - 1) * 5;
        monkeyEnemy.resetEnemy(72 + scale, 20 + dmgScale, "A sharp blade helps before the deeper halls.");
        wraithEnemy.resetEnemy(88 + scale, 24 + dmgScale, "No turning back once higher chambers claim you.");
        golemEnemy.resetEnemy(106 + scale, 27 + dmgScale, "Ancient wardens test strength and relics.");
        sentinelEnemy.resetEnemy(126 + scale, 31 + dmgScale, "A heavy ward key opens the way ahead.");

        r1.setRoomData("1. Courtyard",
            "Steel sometimes waits off the main path before deeper shadows.",
            "Stone courtyard where paths split ahead.",
            &torchItem, nullptr);
        r2.setRoomData("2. East Wing",
            "A sharp edge helps before deeper halls; explore any direction freely.",
            "An eastern side hall with weapon racks and a lurking demon.",
            &bladeItem, &monkeyEnemy);
        r3.setRoomData("3. Lantern Gallery",
            "Save healing for deep wardens, or step back if you need to regroup.",
            "Dim arches lit by flickering wall sconces and wild herbs in the cracks.",
            &herbItem, &wraithEnemy);
        r4.setRoomData("4. Deep Hall",
            "Ancient relics carry unseen strength; both North and East lead toward the sanctum.",
            "A sunken hall with a glowing relic and heavy guardian.",
            &relicItem, &golemEnemy, false, "");
        r5.setRoomData("5. Inner Sanctum",
            "What opens a gate often waits just before it.",
            "Quiet sanctum where an iron ward glints near the altar.",
            &keyItem, &golemEnemy);
        r6.setRoomData("6. Final Gate",
            "Defeat the ward and use the door once fitted.",
            "Towering doors marking the end of the level.",
            nullptr, &sentinelEnemy);

        out << "\n=== LEVEL " << levelNumber << " ===\n";
        currentRoom->describe(out, player.getHealth(), player.getMaxHealth(), player.getStamina());
    }

    std::string step(const std::string& rawInput) {
        std::ostringstream out;
        std::string input = trimStr(rawInput);
        std::string lower = toLowerStr(input);
        lastAnimAction = "idle";

        if (lower.rfind("start ", 0) == 0) {
            std::string rest = trimStr(lower.substr(6));
            int av = 1, lv = 1;
            if (rest.find("lyra") != std::string::npos || rest.find("2") != std::string::npos) av = 2;
            if (rest.find("thorne") != std::string::npos || rest.find("3") != std::string::npos) av = 3;
            if (rest.find("level2") != std::string::npos || rest.find("lvl2") != std::string::npos) lv = 2;
            if (rest.find("level3") != std::string::npos || rest.find("lvl3") != std::string::npos) lv = 3;
            player.configureAvatar(av);
            configureLevel(lv, out);
            return out.str();
        }

        if (lower == "timeout") {
            restartCurrentLevel("Time expired!", out);
            return out.str();
        }

        if (worldDestroyed) {
            lastAnimAction = "apocalypse";
            out << "\n*** CAMPAIGN COMPLETE! ***\n";
            return out.str();
        }

        auto tryMove = [&](Room* nextRoom, const std::string& dirName, int stamCost, const std::string& animName) {
            Enemy* foe = currentRoom->getEnemy();
            if (dirName == "north" && foe && foe->isAlive()) {
                out << "\n[Blocked] Defeat " << foe->getName() << " before pushing North.\n";
                return;
            }
            if (!nextRoom) {
                out << "\n[No Path] There is no route " << dirName << " from here.\n";
                return;
            }
            lastAnimAction = animName;
            currentRoom = nextRoom;
            player.drainMoveStamina(stamCost, out);
            out << "\n[Move " << dirName << "] -" << stamCost << " STM (STM: " << player.getStamina() << ")\n";
            if (!player.isAlive()) {
                restartCurrentLevel("You collapsed from exhaustion!", out);
                return;
            }
            if (currentRoom->isLethal()) {
                restartCurrentLevel(currentRoom->getLethalMsg(), out);
            }
        };

        if (lower == "north") {
            tryMove(currentRoom->getNorth(), "north", 12, "run_north");
        } else if (lower == "south" || lower == "retreat") {
            tryMove(currentRoom->getSouth(), "south", 16, "run_south");
        } else if (lower == "east") {
            tryMove(currentRoom->getEast(), "east", 12, "run_east");
        } else if (lower == "west") {
            tryMove(currentRoom->getWest(), "west", 12, "run_west");
        } else if (lower == "take") {
            Item* it = currentRoom->takeRoomItem();
            if (it) {
                player.getInventory().addItem(it, out);
                lastAnimAction = (it->getName() == "Relic") ? "relic_found" : "take";
            } else {
                out << "\n[Info] Nothing here to take.\n";
            }
        } else if (lower.rfind("use ", 0) == 0) {
            player.useItem(trimStr(input.substr(4)), out);
        } else if (lower == "rest") {
            player.gainStamina(25);
            player.heal(6, out);
            out << "\n[Rest] +25 STM, +6 HP.\n";
        } else if (lower == "search") {
            out << "\n[Hint] " << currentRoom->getHint() << "\n";
        } else if (lower == "slash" || lower == "attack") {
            Enemy* foe = currentRoom->getEnemy();
            if (foe && foe->isAlive()) {
                lastAnimAction = "slash";
                int dmg = player.getAttackPower(levelNumber);
                out << "\n[Slash] " << dmg << " DMG to " << foe->getName() << "!\n";
                foe->takeDamage(dmg, out);
                if (foe->isAlive()) foe->strikePlayer(player, out, false);
                else out << "\n[Defeated] " << foe->getName() << " is down!\n";
            } else {
                out << "\n[Info] No enemy here.\n";
            }
        } else if (lower == "vault") {
            Enemy* foe = currentRoom->getEnemy();
            if (foe && foe->isAlive()) {
                lastAnimAction = "vault";
                if (!player.spendStamina(25)) {
                    out << "\n[Low STM] Need 25 STM for Vault.\n";
                    foe->strikePlayer(player, out, false);
                } else {
                    int dmg = static_cast<int>(player.getAttackPower(levelNumber) * 1.4) + 6;
                    out << "\n[Vault] " << dmg << " DMG to " << foe->getName() << "! (-25 STM)\n";
                    foe->takeDamage(dmg, out);
                    if (foe->isAlive()) foe->strikePlayer(player, out, false);
                    else out << "\n[Defeated] " << foe->getName() << " is down!\n";
                }
            } else {
                out << "\n[Info] No enemy here.\n";
            }
        } else if (lower == "guard") {
            Enemy* foe = currentRoom->getEnemy();
            if (foe && foe->isAlive()) {
                lastAnimAction = "guard";
                player.gainStamina(20);
                int cDmg = player.getAttackPower(levelNumber) / 2;
                out << "\n[Guard] Riposte for " << cDmg << " DMG (+20 STM)!\n";
                foe->takeDamage(cDmg, out);
                if (foe->isAlive()) foe->strikePlayer(player, out, true);
                else out << "\n[Defeated] " << foe->getName() << " is down!\n";
            } else {
                out << "\n[Info] No enemy here.\n";
            }
        } else if (lower == "torch") {
            Enemy* foe = currentRoom->getEnemy();
            if (foe && foe->isAlive()) {
                if (!player.getInventory().hasItem("Torch")) {
                    out << "\n[No Torch] You need a Torch first.\n";
                } else if (!player.spendStamina(20)) {
                    out << "\n[Low STM] Need 20 STM for Torch Flare.\n";
                } else {
                    lastAnimAction = "torch";
                    int dmg = 26 + levelNumber * 4;
                    out << "\n[Torch Flare] " << dmg << " DMG to " << foe->getName() << "! (-20 STM)\n";
                    foe->takeDamage(dmg, out);
                    if (foe->isAlive()) foe->strikePlayer(player, out, false);
                    else out << "\n[Defeated] " << foe->getName() << " is down!\n";
                }
            } else {
                player.useItem("Torch", out);
            }
        } else if (lower == "door") {
            if (currentRoom == &r6) {
                Enemy* foe = currentRoom->getEnemy();
                if (foe && foe->isAlive()) {
                    out << "\n[Gate] Defeat " << foe->getName() << " first.\n";
                } else if (!player.getInventory().hasItem("Key")) {
                    out << "\n[Locked] The gate requires a Key.\n";
                } else if (levelNumber < 3) {
                    out << "\n[Cleared] Advancing to Level " << (levelNumber + 1) << "!\n";
                    configureLevel(levelNumber + 1, out);
                    lastAnimAction = "level_up";
                    return out.str();
                } else {
                    worldDestroyed = true;
                    lastAnimAction = "apocalypse";
                    out << "\n*** VICTORY! ALL 3 LEVELS CONQUERED! ***\n";
                    return out.str();
                }
            } else {
                out << "\n[Info] Reach Chamber 6 (Final Gate) to open the door.\n";
            }
        } else {
            out << "\n[?] Unknown command '" << input << "'.\n";
        }

        if (!player.isAlive()) {
            restartCurrentLevel("You were defeated!", out);
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
        return (foe && foe->isAlive()) ? foe->getHealth() : 0;
    }
    int getEnemyMaxHp() const {
        Enemy* foe = currentRoom->getEnemy();
        return foe ? foe->getMaxHealth() : 0;
    }
    const char* getEnemyNameCstr() const {
        static std::string s;
        Enemy* foe = currentRoom->getEnemy();
        s = (foe && foe->isAlive()) ? foe->getName() : "";
        return s.c_str();
    }
    const char* getEnemyKindCstr() const {
        static std::string s;
        Enemy* foe = currentRoom->getEnemy();
        s = (foe && foe->isAlive()) ? foe->getKind() : "";
        return s.c_str();
    }
    const char* getPromptModeCstr() const { return ""; }
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
EXPORT const char* current_room() { static std::string s; s = g_engine.getRoomId(); return s.c_str(); }
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
