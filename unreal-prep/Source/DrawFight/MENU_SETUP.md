# ติดตั้งเมนู / เลือกตัวละคร / กาชา (โค้ดชุดใหม่ ยังไม่เคยคอมไพล์)

ไฟล์ใหม่: `DFSaveGame.h`, `DFProfileSubsystem.h/.cpp`, `DFMenuWidget.h/.cpp`, `DFMenuGameMode.h/.cpp`
แก้ไฟล์เดิม: `DFDataTypes.h` (เพิ่ม FDFCharacterRow), `DFArenaGameMode.cpp` (ใส่สเตตัสตัวละคร + ให้ทองตอนจบ), `DrawFight.Build.cs` (เพิ่ม UMG, Slate, SlateCore)

1. ก๊อปไฟล์ทั้งหมดลง `Source/DrawFight/DrawFightCore/` แล้ว Build
2. Content Browser สร้างโฟลเดอร์ `DrawFight/Data` แล้วลาก CSV เข้า:
   - DT_Characters.csv → เลือก Row Type `DFCharacterRow`
   - DT_Skills.csv → `DFSkillRow`
   - DT_WeaponClasses.csv → `DFWeaponClassRow`
   - DT_Elements.csv → `DFElementRow`
   - DT_Auras.csv → `DFAuraRow`
   - DT_SecretClasses.csv → `DFSecretClassRow`
   (ชื่อ asset ต้องตรงตามนี้ โค้ดโหลดจาก `/Game/DrawFight/Data/<ชื่อ>`)
3. สร้างเลเวลว่างชื่อ `MainMenu` → World Settings → GameMode Override = `DFMenuGameMode`
4. Project Settings → Maps & Modes → Game Default Map = `MainMenu`
5. เลเวลต่อสู้ (ค่าเริ่มต้นชื่อ `ThirdPersonMap`) ต้องใช้ GameMode ที่สืบจาก `DFArenaGameMode`
6. ถ้าตัวหนังสือไทยเป็นกล่องสี่เหลี่ยม: import ฟอนต์ไทย .ttf → สร้าง Blueprint ลูกของ DFMenuWidget ตั้ง ThaiFont
   → สร้าง Blueprint ลูกของ DFMenuPlayerController ตั้ง MenuClass → ใส่ใน GameMode

เซฟอยู่ที่ `Saved/SaveGames/DrawFight.sav` ลบไฟล์นี้ = เริ่มโปรไฟล์ใหม่
ปุ่ม "+1000 ทอง (ทดสอบ)" จะหายเองตอน build แบบ Shipping

## HUD ตอนต่อสู้ (DFHudWidget)
เลเวลที่ใช้ DFArenaGameMode จะเริ่มสู้เองหลังโหลด 1 วินาที และขึ้น HUD ให้อัตโนมัติ:
- มุมซ้ายบน: ชื่อตัวละครและหลอดเลือด · กลางบน: ด่าน/ศัตรูเหลือ และหลอดเลือดบอส
- ป้าย "ด่านที่ 2" / "บอสมาแล้ว!" · ตัวเลขดาเมจ/ฮีลลอยขึ้นจากตัว · หลอดเลือดเล็กบนหัวศัตรู (เปลี่ยนเป็นสีส้มตอนกำลังง้างตี)
- ล่าง: ช่องสกิล 1-2-3 และสถานะหลบ (Shift)
- จบเกม: หน้าจอ ชนะ!/แพ้... + ทองที่ได้ + ปุ่ม สู้อีกรอบ / กลับเมนู
ปิดการเริ่มเองได้ที่ bAutoStart ใน GameMode (แล้วเรียก StartFight จาก Blueprint แทน)

## เสียงพากย์ของผู้เล่น
ไม่ต้องตั้งค่าอะไรเพิ่ม `DFVoiceSubsystem` ทำงานเอง · รายละเอียดดู VOICE_PACKS.md
