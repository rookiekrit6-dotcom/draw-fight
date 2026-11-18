# ติดตั้งเมนู / เลือกตัวละคร / กาชา (โค้ดชุดใหม่ ยังไม่เคยคอมไพล์)

ไฟล์ใหม่: `DFSaveGame.h`, `DFProfileSubsystem.h/.cpp`, `DFMenuWidget.h/.cpp`, `DFMenuGameMode.h/.cpp`
แก้ไฟล์เดิม: `DFDataTypes.h` (เพิ่ม FDFCharacterRow), `DFArenaGameMode.cpp` (ใส่สเตตัสตัวละคร + ให้ทองตอนจบ), `DrawFight.Build.cs` (เพิ่ม UMG, Slate, SlateCore)

1. ก๊อปไฟล์ทั้งหมดลง `Source/DrawFight/DrawFightCore/` แล้ว Build
2. Content Browser สร้างโฟลเดอร์ `DrawFight/Data` แล้วลาก CSV เข้า:
   - DT_Characters.csv → เลือก Row Type `DFCharacterRow`
   - DT_Skills.csv → `DFSkillRow`
   - DT_WeaponClasses.csv → `DFWeaponClassRow`
   - DT_Elements.csv → `DFElementRow`
   (ชื่อ asset ต้องตรงตามนี้ โค้ดโหลดจาก `/Game/DrawFight/Data/<ชื่อ>`)
3. สร้างเลเวลว่างชื่อ `MainMenu` → World Settings → GameMode Override = `DFMenuGameMode`
4. Project Settings → Maps & Modes → Game Default Map = `MainMenu`
5. เลเวลต่อสู้ (ค่าเริ่มต้นชื่อ `ThirdPersonMap`) ต้องใช้ GameMode ที่สืบจาก `DFArenaGameMode`
6. ถ้าตัวหนังสือไทยเป็นกล่องสี่เหลี่ยม: import ฟอนต์ไทย .ttf → สร้าง Blueprint ลูกของ DFMenuWidget ตั้ง ThaiFont
   → สร้าง Blueprint ลูกของ DFMenuPlayerController ตั้ง MenuClass → ใส่ใน GameMode

เซฟอยู่ที่ `Saved/SaveGames/DrawFight.sav` ลบไฟล์นี้ = เริ่มโปรไฟล์ใหม่
ปุ่ม "+1000 ทอง (ทดสอบ)" จะหายเองตอน build แบบ Shipping
