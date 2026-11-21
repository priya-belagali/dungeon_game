#include <iostream>
#include <string>
#include <vector>
#include <memory>
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
    TorchItem() : Item("Torch", "Flare Torch: pierces shadows, boosts stamina recovery & sears foes.") {}
    bool use(Player& player, std::ostringstream& out) override;
};

class HerbItem : public Item {
private:
    int healAmount;
public:
    HerbItem(int amt = 42) : Item("Herb", "Crimson Herb: restores vitality in high-stakes rooms."), healAmount(amt) {}
    bool use(Player& player, std::ostringstream& out) override;
};

class BladeItem : public Item {
public:
    BladeItem() : Item("Blade", "Level weapon: Sunlit Blade (Lv.1) / Arcane Glaive (Lv.2) / Eclipse Scythe (Lv.3).") {}
    bool use(Player&, std::ostringstream& out) override {
        out << "\n[Weapon] Your level weapon is equipped and ready for combat!\n";
        return false;
    }
};

class KeyItem : public Item {
public:
    KeyItem() : Item("Key", "Seal Key: unlocks the chamber's Final Gate.") {}
    bool use(Player&, std::ostringstream& out) override {
        out << "\n[Key] Use 'door' at the Final Gate when ready.\n";
        return false;
    }
};

class RelicItem : public Item {
public:
    RelicItem() : Item("Relic", "Ancient Relic: channels forbidden power across deeper stages.") {}
    bool use(Player&, std::ostringstream& out) override {
        out << "\n[Relic] The Relic hums with raw energy!\n";
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
        out << "\n[Acquired] Added [" << item->getName() << "] — " << item->getDescription() << "\n";
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
            out << " [Exhausted] Stamina hit 0! Moving while exhausted costs 8 HP!\n";
            takeDamage(8, out);
        }
    }

    void gainStamina(int amount) {
        stamina = std::min(maxStamina, stamina + amount);
    }

    Inventory& getInventory() { return inventory; }
    const Inventory& getInventory() const { return inventory; }

    int getAttackPower(int levelNumber) const {
        int total = baseAttack + (levelNumber - 1) * 5;
        if (inventory.hasItem("Blade")) total += 18 + (levelNumber * 4);
        if (inventory.hasItem("Torch")) total += 7;
        if (inventory.hasItem("Relic")) total += (levelNumber == 3 ? 32 : 20);
        return total;
    }

    void useItem(const std::string& itemName, std::ostringstream& out) {
        Item* item = inventory.getItem(itemName);
        if (!item) {
            out << "\n[!] No '" << itemName << "' in your pack.\n";
            return;
        }
        if (item->use(*this, out)) {
            inventory.removeItem(itemName);
        }
    }
};

bool TorchItem::use(Player& player, std::ostringstream& out) {
    player.gainStamina(35);
    out << "\n[Torch] You flare the Torch to steady your breathing (+35 STM).\n";
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
        if (guarded) out << " [Guard] Your stance cuts the strike in half!\n";
        player.takeDamage(dmg, out);
    }
};

class DemonMonkeyVillain : public Enemy {
public:
    DemonMonkeyVillain() : Enemy("Demon Monkey", "monkey", 66, 19, "Screech! Wrong step and the ruins bite!") {}
};
class ShadowWraithVillain : public Enemy {
public:
    ShadowWraithVillain() : Enemy("Shadow Wraith", "wraith", 82, 23, "Cold mist drains the careless!") {}
};
class IronGolemVillain : public Enemy {
public:
    IronGolemVillain() : Enemy("Obsidian Golem", "golem", 98, 26, "Stone and fury guard this path!") {}
};
class AstralSentinelVillain : public Enemy {
public:
    AstralSentinelVillain() : Enemy("Astral Warden", "seraph", 118, 30, "None pass unpunished!") {}
};

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
    Room* eastRoom;
    Room* westRoom;
    bool deadEndRoom;
    bool lethalWrongTurn;
    std::string lethalMsg;
    std::string riddlePrompt;
    std::string riddleAnswer;
    bool riddleSolved;
public:
    Room(std::string roomId)
        : id(roomId), roomItem(nullptr), enemy(nullptr),
          northRoom(nullptr), southRoom(nullptr), eastRoom(nullptr), westRoom(nullptr),
          deadEndRoom(false), lethalWrongTurn(false), riddleSolved(false) {}

    const std::string& getId() const { return id; }
    const std::string& getTitle() const { return title; }
    const std::string& getObjective() const { return objectiveHint; }
    bool isDeadEnd() const { return deadEndRoom; }
    bool isLethal() const { return lethalWrongTurn; }
    const std::string& getLethalMsg() const { return lethalMsg; }
    bool hasRiddle() const { return !riddlePrompt.empty(); }
    bool isRiddleSolved() const { return riddleSolved || riddlePrompt.empty(); }
    const std::string& getRiddlePrompt() const { return riddlePrompt; }
    void resetRiddle() { riddleSolved = false; }

    bool solveRiddle(const std::string& ans, Player& player, std::ostringstream& out) {
        std::string c = toLowerStr(trimStr(ans));
        if (c == riddleAnswer) {
            riddleSolved = true;
            out << "\n[Solved] The rune lock disengages!\n";
            return true;
        }
        out << "\n[Wrong Answer] Trap runes discharge for 20 DMG!\n";
        player.takeDamage(20, out);
        return false;
    }

    void setRoomData(std::string t, std::string hint, std::string desc, Item* item, Enemy* foe,
                     bool deadEnd = false, bool lethal = false, std::string lMsg = "",
                     std::string rPrompt = "", std::string rAns = "") {
        title = t; objectiveHint = hint; description = desc;
        roomItem = item; enemy = foe;
        deadEndRoom = deadEnd; lethalWrongTurn = lethal; lethalMsg = lMsg;
        riddlePrompt = rPrompt; riddleAnswer = toLowerStr(rAns);
        riddleSolved = riddlePrompt.empty();
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
        out << "\n[" << title << "] HP " << hp << "/" << maxHp << " | STM " << stm << " — " << objectiveHint << "\n";
        out << description << "\n";
        std::string exits = "Exits:";
        if (northRoom) exits += " [North]";
        if (southRoom) exits += " [South]";
        if (eastRoom)  exits += " [East]";
        if (westRoom)  exits += " [West]";
        out << " " << exits << "\n";
        if (roomItem) out << " * Item: [" << roomItem->getName() << "] ('take')\n";
        if (enemy && enemy->isAlive()) {
            out << " * Guardian: " << enemy->getName() << " (" << enemy->getHealth() << "/" << enemy->getMaxHealth() << " HP)\n";
        }
        if (!riddlePrompt.empty() && !riddleSolved) {
            out << " * Seal Riddle ('search'): " << riddlePrompt << "\n";
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
    RelicItem relicItem;

    DemonMonkeyVillain monkeyEnemy;
    ShadowWraithVillain wraithEnemy;
    IronGolemVillain golemEnemy;
    AstralSentinelVillain sentinelEnemy;

    // Maze nodes: main progression + side branches (dead ends & lethal wrong turns)
    Room rStart, rMaze1, rDeadWest, rPitEast, rRelicVault, rGate;
    Room* currentRoom;
    int levelNumber;
    int defeatsCount;
    bool worldDestroyed;
    bool awaitingRiddle;
    std::string lastAnimAction;

    void wireMaze() {
        // rStart -> (North) rMaze1, (East) rPitEast (lethal wrong turn), (West) rDeadWest (dead end)
        // rMaze1 -> (South) rStart, (North) rRelicVault, (West) rDeadWest, (East) rPitEast
        // rRelicVault -> (South) rMaze1, (North) rGate, (East) rPitEast
        // rGate -> (South) rRelicVault, (West) rDeadWest
        rStart.setExits(&rMaze1, nullptr, &rPitEast, &rDeadWest);
        rDeadWest.setExits(nullptr, nullptr, &rStart, nullptr);
        rPitEast.setExits(nullptr, nullptr, nullptr, &rStart);
        rMaze1.setExits(&rRelicVault, &rStart, &rPitEast, &rDeadWest);
        rRelicVault.setExits(&rGate, &rMaze1, &rPitEast, nullptr);
        rGate.setExits(nullptr, &rRelicVault, nullptr, &rDeadWest);
    }

    void restartCurrentLevel(const std::string& reason, std::ostringstream& out) {
        defeatsCount++;
        lastAnimAction = "defeat";
        out << "\n[LEVEL RESTARTED] " << reason << "\n";
        configureLevel(levelNumber, out);
    }

public:
    GameEngine()
        : herbItem1(42), herbItem2(48),
          rStart("cell"), rMaze1("corridor"), rDeadWest("cistern"),
          rPitEast("forge"), rRelicVault("sanctum"), rGate("gate"),
          currentRoom(&rStart), levelNumber(1), defeatsCount(0),
          worldDestroyed(false), awaitingRiddle(false), lastAnimAction("idle") {
        wireMaze();
    }

    void configureLevel(int lvl, std::ostringstream& out) {
        levelNumber = std::max(1, std::min(3, lvl));
        player.configureAvatar(player.getAvatarId());
        awaitingRiddle = false;
        worldDestroyed = false;
        lastAnimAction = "idle";
        currentRoom = &rStart;

        if (levelNumber == 1) {
            // Level 1: Sunlit Ruins — Weapon: Sunlit Blade — Objective: Claim Torch & Blade, find Key & Relic, unlock Exit Gate
            monkeyEnemy.resetEnemy(64, 18, "Ruin screech! Watch your footing!");
            wraithEnemy.resetEnemy(76, 21, "Mist chills your bones!");
            golemEnemy.resetEnemy(92, 24, "Stone guards the temple path!");
            sentinelEnemy.resetEnemy(105, 27, "Only worthy delvers pass!");

            rStart.setRoomData("1. Sunlit Courtyard",
                "Lv.1 Objective: Grab Torch, navigate maze North to unlock the Exit Gate.",
                "Temple courtyard. Weapon focus: Sunlit Blade. East looks risky; West is a blind passage.",
                &torchItem, nullptr);
            rDeadWest.setRoomData("Dead End: Sunken Alcove",
                "Dead end! Turn East/South to backtrack (drains stamina).",
                "A flooded dead end with no northern exit.",
                &herbItem1, &wraithEnemy, true, false);
            rPitEast.setRoomData("Lethal Trap: Collapse Chasm",
                "Wrong turn! Spike pit & collapse!",
                "Unstable floor gives way into jagged spikes!",
                nullptr, nullptr, false, true, "You took a lethal wrong turn into the Collapse Chasm!");
            rMaze1.setRoomData("2. Shifting Gallery",
                "Claim Sunlit Blade, defeat Demon Monkey, push North or East/West carefully.",
                "Moving statues and dart slits guard this gallery.",
                &bladeItem, &monkeyEnemy);
            rRelicVault.setRoomData("3. Relic Shrine",
                "Claim Key & Relic, solve seal riddle ('search') if needed, head North to Gate.",
                "Ancient pedestal holding the Key and Relic.",
                &keyItem, &golemEnemy, false, false, "",
                "What gets wetter the more it dries?", "towel");
            rGate.setRoomData("4. Level 1 Exit Gate",
                "Use 'door' with Key to advance to Level 2.",
                "Heavy stone gate sealed with ancient runes.",
                &relicItem, &sentinelEnemy);
        } else if (levelNumber == 2) {
            // Level 2: Relic Hunt — Weapon: Arcane Glaive — Objective: Claim Arcane Glaive & Relic, survive maze & unlock Gate
            monkeyEnemy.resetEnemy(74, 22, "Relic hunters face sharper claws!");
            wraithEnemy.resetEnemy(88, 25, "Wraiths swarm the dark corridors!");
            golemEnemy.resetEnemy(104, 28, "The Golem blocks the Relic!");
            sentinelEnemy.resetEnemy(122, 31, "The Warden bars the deeper gate!");

            rStart.setRoomData("1. Relic Approach",
                "Lv.2 Objective: Claim Arcane Glaive & Relic, reach Gate without running out of stamina.",
                "Deeper catacombs. Weapon focus: Arcane Glaive. Wrong turns are deadlier.",
                &bladeItem, nullptr);
            rDeadWest.setRoomData("Dead End: Whispering Well",
                "Dead end! Backtrack carefully—running drains stamina.",
                "A dark dead-end cistern with a lurking wraith.",
                &herbItem1, &wraithEnemy, true, false);
            rPitEast.setRoomData("Lethal Trap: Blade Pendulum",
                "Wrong turn! Swinging blades strike fatally!",
                "Razor pendulums slice across the eastern corridor!",
                nullptr, nullptr, false, true, "Wrong turn! The Blade Pendulum cut your run short!");
            rMaze1.setRoomData("2. Mirror Maze",
                "Defeat Demon Monkey & solve the mirror riddle ('search').",
                "Intersecting hallways where false reflections mislead runners.",
                &herbItem2, &monkeyEnemy, false, false, "",
                "I speak without a mouth and hear without ears. What am I?", "echo");
            rRelicVault.setRoomData("3. Sanctum of the Relic",
                "Claim [Relic] & [Key], defeat Golem, advance North to Gate.",
                "The Ancient Relic pulses with violet power.",
                &relicItem, &golemEnemy);
            rGate.setRoomData("4. Level 2 Gate",
                "Use 'door' with Key & Relic to enter Level 3.",
                "Runic gate guarding the path to Level 3.",
                &keyItem, &sentinelEnemy);
        } else {
            // Level 3: Eclipse Endgame — Weapon: Eclipse Scythe — Objective: Survive timer & maze, unlock World-Shattering Gate
            monkeyEnemy.resetEnemy(82, 25, "Stop the Eclipse Scythe!");
            wraithEnemy.resetEnemy(96, 28, "Shadows rise against your endgame!");
            golemEnemy.resetEnemy(114, 32, "End this dark plot!");
            sentinelEnemy.resetEnemy(134, 35, "Final stand before the altar!");

            rStart.setRoomData("1. Eclipse Threshold",
                "Lv.3 Objective: Beat the clock & maze—grab Eclipse Scythe, Relic, Key & open Final Gate!",
                "Endgame maze under a timed countdown. Weapon focus: Eclipse Scythe.",
                &bladeItem, nullptr);
            rDeadWest.setRoomData("Dead End: Void Trench",
                "Dead end! Backtracking costs stamina—watch your meter!",
                "A dead-end trench haunted by a wraith.",
                &herbItem1, &wraithEnemy, true, false);
            rPitEast.setRoomData("Lethal Trap: Abyssal Rift",
                "Wrong turn! Void rift swallows careless runners!",
                "A tearing void fissure spans the eastern turn!",
                nullptr, nullptr, false, true, "Wrong turn! You fell into the Abyssal Rift!");
            rMaze1.setRoomData("2. Labyrinth of Ash",
                "Defeat Demon Monkey, claim Relic & solve riddle ('search').",
                "Ash-choked labyrinth twisting toward the endgame altar.",
                &relicItem, &monkeyEnemy, false, false, "",
                "The more you take from me, the bigger I grow. What am I?", "hole");
            rRelicVault.setRoomData("3. Crucible Vault",
                "Claim Key & Herb, defeat Golem, head North to Final Gate.",
                "Final crucible before the endgame gate.",
                &keyItem, &golemEnemy);
            rGate.setRoomData("4. Endgame Final Gate",
                "Use 'door' to trigger the final gate sequence!",
                "The culminating gate of the campaign.",
                &herbItem2, &sentinelEnemy);
        }

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
            restartCurrentLevel("Time ran out on Level 3!", out);
            return out.str();
        }

        if (worldDestroyed) {
            lastAnimAction = "apocalypse";
            out << "\n*** CAMPAIGN COMPLETE! ***\n";
            return out.str();
        }

        if (awaitingRiddle) {
            awaitingRiddle = false;
            currentRoom->solveRiddle(input, player, out);
            if (!player.isAlive()) {
                restartCurrentLevel("You fell to a trap rune!", out);
                return out.str();
            }
            currentRoom->describe(out, player.getHealth(), player.getMaxHealth(), player.getStamina());
            return out.str();
        }

        auto tryMove = [&](Room* nextRoom, const std::string& dirName, int stamCost, const std::string& animName) {
            Enemy* foe = currentRoom->getEnemy();
            if (dirName == "north" && foe && foe->isAlive()) {
                out << "\n[Blocked] " << foe->getName() << " guards the northern path! Fight first.\n";
                return;
            }
            if (dirName == "north" && !currentRoom->isRiddleSolved()) {
                out << "\n[Sealed] Solve the room riddle ('search') before moving North.\n";
                return;
            }
            if (!nextRoom) {
                out << "\n[Wall] Solid stone blocks " << dirName << ".\n";
                return;
            }
            lastAnimAction = animName;
            currentRoom = nextRoom;
            player.drainMoveStamina(stamCost, out);
            out << "\n[Move " << dirName << "] -" << stamCost << " STM (Remaining: " << player.getStamina() << ")\n";
            if (!player.isAlive()) {
                restartCurrentLevel("Exhaustion and wounds overcame you!", out);
                return;
            }
            if (currentRoom->isLethal()) {
                restartCurrentLevel(currentRoom->getLethalMsg(), out);
                return;
            }
            if (currentRoom->isDeadEnd()) {
                out << "[Dead End] You hit a dead end! You must turn back.\n";
            }
        };

        if (lower == "north") {
            tryMove(currentRoom->getNorth(), "north", 12, "run_north");
        } else if (lower == "south" || lower == "retreat") {
            // Running south / backtracking drains extra stamina as requested
            tryMove(currentRoom->getSouth(), "south", 18, "run_south");
        } else if (lower == "east") {
            tryMove(currentRoom->getEast(), "east", 14, "run_north");
        } else if (lower == "west") {
            tryMove(currentRoom->getWest(), "west", 14, "run_north");
        } else if (lower == "take") {
            Item* it = currentRoom->takeRoomItem();
            if (it) {
                player.getInventory().addItem(it, out);
                lastAnimAction = (it->getName() == "Relic") ? "relic_found" : "take";
            } else {
                out << "\n[Info] Nothing left to take here.\n";
            }
        } else if (lower.rfind("use ", 0) == 0) {
            player.useItem(trimStr(input.substr(4)), out);
        } else if (lower == "rest") {
            player.gainStamina(32);
            player.heal(10, out);
            out << "\n[Rest] You catch your breath (+32 STM, +10 HP).\n";
        } else if (lower == "search") {
            if (currentRoom->hasRiddle() && !currentRoom->isRiddleSolved()) {
                awaitingRiddle = true;
                out << "\n[Riddle] " << currentRoom->getRiddlePrompt() << "\n";
            } else {
                out << "\n[Inspect] Nothing new to unlock here.\n";
            }
        } else if (lower == "slash" || lower == "attack") {
            Enemy* foe = currentRoom->getEnemy();
            if (foe && foe->isAlive()) {
                lastAnimAction = "slash";
                int dmg = player.getAttackPower(levelNumber);
                out << "\n[Attack] " << player.getName() << " strikes " << foe->getName() << " for " << dmg << " DMG!\n";
                foe->takeDamage(dmg, out);
                if (foe->isAlive()) foe->strikePlayer(player, out, false);
                else out << "\n[Defeated] " << foe->getName() << " falls!\n";
            } else {
                out << "\n[Info] No foe here.\n";
            }
        } else if (lower == "vault") {
            Enemy* foe = currentRoom->getEnemy();
            if (foe && foe->isAlive()) {
                lastAnimAction = "vault";
                if (!player.spendStamina(25)) {
                    out << "\n[Low Stamina] Need 25 STM to vault!\n";
                    foe->strikePlayer(player, out, false);
                } else {
                    int dmg = static_cast<int>(player.getAttackPower(levelNumber) * 1.45) + 8;
                    out << "\n[Vault] Heavy vault strike for " << dmg << " DMG! (-25 STM)\n";
                    foe->takeDamage(dmg, out);
                }
            } else {
                out << "\n[Info] No foe here.\n";
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
            } else {
                out << "\n[Info] No foe here.\n";
            }
        } else if (lower == "torch") {
            Enemy* foe = currentRoom->getEnemy();
            if (foe && foe->isAlive()) {
                if (!player.getInventory().hasItem("Torch")) {
                    out << "\n[No Torch] Claim the Torch first!\n";
                } else if (!player.spendStamina(20)) {
                    out << "\n[Low Stamina] Need 20 STM for Torch Flare!\n";
                } else {
                    lastAnimAction = "torch";
                    int dmg = 28 + levelNumber * 4;
                    out << "\n[Torch Flare] Sears " << foe->getName() << " for " << dmg << " DMG! (-20 STM)\n";
                    foe->takeDamage(dmg, out);
                    if (foe->isAlive()) player.takeDamage(foe->getAttackDamage() / 2, out);
                }
            } else {
                player.useItem("Torch", out);
            }
        } else if (lower == "door") {
            if (currentRoom == &rGate) {
                if (!player.getInventory().hasItem("Key")) {
                    out << "\n[Locked] You need [Key] to open this gate.\n";
                } else if (levelNumber == 1) {
                    out << "\n[Level 1 Cleared!] Advancing to Level 2!\n";
                    configureLevel(2, out);
                    lastAnimAction = "level_up";
                    return out.str();
                } else if (levelNumber == 2) {
                    out << "\n[Level 2 Cleared!] Advancing to Level 3!\n";
                    configureLevel(3, out);
                    lastAnimAction = "twist";
                    return out.str();
                } else {
                    worldDestroyed = true;
                    lastAnimAction = "apocalypse";
                    out << "\n*** VICTORY! ALL 3 LEVELS CONQUERED! ***\n";
                    return out.str();
                }
            } else {
                out << "\n[Info] Reach the Gate chamber to use 'door'.\n";
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
    const char* getPromptModeCstr() const {
        static std::string s;
        s = awaitingRiddle ? "riddle_gate" : "";
        return s.c_str();
    }
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
