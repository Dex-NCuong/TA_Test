# -*- coding: utf-8 -*-
"""Sinh tài liệu phong van (.docx) tieng Viet cho bai test Technical Artist D/E/F."""

from docx import Document
from docx.shared import Pt, RGBColor, Inches
from docx.enum.text import WD_ALIGN_PARAGRAPH
from docx.enum.table import WD_TABLE_ALIGNMENT
from docx.oxml.ns import qn
from docx.oxml import OxmlElement

DARK = RGBColor(0x1F, 0x2A, 0x44)
ACCENT = RGBColor(0x2E, 0x5C, 0x8A)
GREY = RGBColor(0x55, 0x55, 0x55)

doc = Document()

# ---- Base styles ----
normal = doc.styles["Normal"]
normal.font.name = "Calibri"
normal.font.size = Pt(11)
normal.element.rPr.rFonts.set(qn("w:eastAsia"), "Calibri")

for i, sz in [(1, 18), (2, 14), (3, 12)]:
    st = doc.styles[f"Heading {i}"]
    st.font.name = "Calibri"
    st.font.size = Pt(sz)
    st.font.color.rgb = DARK if i == 1 else ACCENT
    st.font.bold = True


def add_bullet(text, bold_lead=None):
    p = doc.add_paragraph(style="List Bullet")
    if bold_lead:
        r = p.add_run(bold_lead)
        r.bold = True
        p.add_run(text)
    else:
        p.add_run(text)
    return p


def add_para(text, italic=False, color=None, size=None):
    p = doc.add_paragraph()
    r = p.add_run(text)
    r.italic = italic
    if color:
        r.font.color.rgb = color
    if size:
        r.font.size = Pt(size)
    return p


def add_code(text):
    p = doc.add_paragraph()
    r = p.add_run(text)
    r.font.name = "Consolas"
    r.font.size = Pt(9.5)
    r.font.color.rgb = RGBColor(0x33, 0x33, 0x33)
    pPr = p._p.get_or_add_pPr()
    shd = OxmlElement("w:shd")
    shd.set(qn("w:val"), "clear")
    shd.set(qn("w:fill"), "F2F2F2")
    pPr.append(shd)
    return p


def shade_cell(cell, hexfill):
    tcPr = cell._tc.get_or_add_tcPr()
    shd = OxmlElement("w:shd")
    shd.set(qn("w:val"), "clear")
    shd.set(qn("w:fill"), hexfill)
    tcPr.append(shd)


def add_table(headers, rows, widths=None):
    t = doc.add_table(rows=1, cols=len(headers))
    t.style = "Table Grid"
    t.alignment = WD_TABLE_ALIGNMENT.CENTER
    hdr = t.rows[0].cells
    for i, h in enumerate(headers):
        hdr[i].text = ""
        p = hdr[i].paragraphs[0]
        r = p.add_run(h)
        r.bold = True
        r.font.color.rgb = RGBColor(0xFF, 0xFF, 0xFF)
        r.font.size = Pt(10)
        shade_cell(hdr[i], "2E5C8A")
    for row in rows:
        cells = t.add_row().cells
        for i, val in enumerate(row):
            cells[i].text = ""
            p = cells[i].paragraphs[0]
            r = p.add_run(str(val))
            r.font.size = Pt(10)
    if widths:
        for row in t.rows:
            for i, w in enumerate(widths):
                row.cells[i].width = Inches(w)
    return t


# ============ TRANG BÌA ============
title = doc.add_paragraph()
title.alignment = WD_ALIGN_PARAGRAPH.CENTER
r = title.add_run("TÀI LIỆU PHỎNG VẤN — TECHNICAL ARTIST")
r.bold = True
r.font.size = Pt(24)
r.font.color.rgb = DARK

sub = doc.add_paragraph()
sub.alignment = WD_ALIGN_PARAGRAPH.CENTER
r = sub.add_run("Bài test D + E + F — Unreal Engine 5.8")
r.font.size = Pt(14)
r.font.color.rgb = ACCENT

for line in [
    "Ứng viên: Đào Nhật Cường",
    "Vị trí: Technical Artist",
    "Nội dung: Chức năng của dự án và cách xây dựng (kèm cách làm plugin C++)",
]:
    p = doc.add_paragraph()
    p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    r = p.add_run(line)
    r.font.size = Pt(11)
    r.font.color.rgb = GREY

doc.add_paragraph()

# ============ 1. TỔNG QUAN ============
doc.add_heading("1. Tổng quan dự án", level=1)
add_para(
    "Dự án gồm ba phần độc lập, mỗi phần thể hiện một nhóm năng lực Technical Artist "
    "khác nhau, tất cả được xây dựng trong Unreal Engine 5.8:"
)
add_table(
    ["Bài test", "Trọng tâm", "Sản phẩm chính"],
    [
        ["D — Shader Tech", "Master Material + hệ thống thời tiết",
         "Một master material điều khiển bởi Material Parameter Collection toàn cục (độ ẩm, tuyết, nhiệt độ) + 6 material instance bề mặt."],
        ["E — Gameplay Tech", "Kiến trúc Blueprint + ability data-driven",
         "3 ability (Fireball, Heal, Teleport) trên hệ thống ActorComponent modular, dùng Gameplay Tags, Data Assets, Enhanced Input, ability interface chung."],
        ["F — Pipeline", "Editor Utility Widget + plugin C++",
         "Wizard 5 bước tạo enemy tự động: đặt tên, gán skeletal mesh/material, cấu hình chỉ số, gán Behavior Tree, sinh Blueprint. EUW giữ layout, plugin C++ 2 module giữ logic."],
    ],
    widths=[1.4, 1.9, 3.7],
)

# ============ 2. TEST D ============
doc.add_heading("2. Test D — Hệ thống Material thời tiết động", level=1)

doc.add_heading("2.1 Chức năng", level=2)
add_bullet("— 5 tham số: 4 texture (BaseColor, Roughness, Normal, Height) + static switch UseParallax. Thời tiết KHÔNG nằm trong tham số material.", "Master Material (M_SurfaceMaster) ")
add_bullet("— 4 hàm tái sử dụng: MF_WetnessSurface (độ ẩm theo hướng normal), MF_SnowAccumulation (tuyết bám mặt hướng lên), MF_TemperatureEffects (đóng băng dưới 0°C), MF_ParallaxOcclusion (BumpOffset, không phải POM ray-march).", "Material Function Library ")
add_bullet("— Stone, Wood, Metal, Fabric, Ice, Dirt, dùng chung master, mỗi cái một bộ texture PBR.", "6 Material Instance bề mặt ")
add_bullet("— WetnessAmount (0–1), SnowAmount (0–1), Temperature (mặc định 20°C), thêm SunColor / SkyTint cho chu kỳ ngày–đêm.", "MPC_GlobalWeather ")
add_bullet("— điều khiển giá trị MPC và nội suy chuyển tiếp thời tiết; kèm BP_WeatherMaterialPreviewRig để so sánh bề mặt cạnh nhau.", "BP_WeatherController ")

doc.add_heading("2.2 Cách làm", level=2)
add_para(
    "Điểm mấu chốt: thời tiết được đưa vào qua 4 node CollectionParameter đọc từ "
    "MPC_GlobalWeather, thay vì phơi ra thành tham số material. Nhờ vậy mỗi khi thay đổi "
    "thời tiết, shader KHÔNG bị recompile và KHÔNG sinh thêm material instance — chỉ cập nhật "
    "một vài giá trị scalar/vector toàn cục."
)
add_bullet("Chuyển tiếp thời tiết dùng easing Smoothstep để vào–ra mượt, thay cho FInterpTo thô:")
add_code("easedAlpha = t * t * (3 - 2 * t)   với t = progress / duration")
add_para(
    "Khi bắt đầu chuyển tiếp mới (PreviewRain, PreviewSnow…), trạng thái hiện tại được lưu vào "
    "các biến Old*, bộ đếm progress reset, nên chuyển thời tiết giữa chừng vẫn redirect mượt từ "
    "trạng thái đang nội suy."
)
add_bullet("Hệ thống thời gian trong ngày: SunColor 4 keyframe (Đêm → Bình minh → Trưa → Hoàng hôn), SkyTint 4 keyframe bổ trợ, cường độ ngày–đêm theo đường sin, và điều khiển Directional Light (UseTemperature/Temperature/Intensity).")

doc.add_heading("2.3 Điểm cần nói thẳng khi phỏng vấn", level=2)
add_para(
    "Các bản mobile (MI_Weather_*_Mobile) hiện chỉ là scaffolding: đã parent về bản desktop "
    "nhưng CHƯA có override nào, nên compile ra cùng shader — chưa có khác biệt chi phí. "
    "UseParallax đang tắt trên cả 12 instance nên nhánh Height/BumpOffset bị compile out. "
    "Đây là bước tiếp theo có chủ đích để lại, không thổi phồng thành đã tối ưu.",
    italic=True, color=GREY,
)

# ============ 3. TEST E ============
doc.add_heading("3. Test E — Hệ thống Ability modular", level=1)

doc.add_heading("3.1 Chức năng", level=2)
add_bullet("— ActorComponent quản lý ability đang hoạt động, timer cooldown, và input binding.", "BP_AbilitySystemComponent ")
add_bullet("— Object Blueprint với hàm ảo CanActivate, Execute, OnEnd và các slot dữ liệu Cooldown, Cost, Duration.", "BP_AbilityBase ")
add_bullet("Fireball (projectile + Niagara NS_Free_Magic_Fire2), Heal (hồi máu tức thì), Teleport (line-trace di chuyển).", "3 ability: ")
add_bullet("— tag phân cấp (Ability.Magic.Fire.Fireball…) để lọc ability và phân loại cooldown, cấu hình qua DefaultGameplayTags.ini.", "Gameplay Tags ")
add_bullet("— DA_AbilityData chứa DA_Fireball / DA_Heal / DA_Teleport, truy vấn dữ liệu qua Blueprint.", "Data Assets ")
add_bullet("BPI_Ability chuẩn hóa giao tiếp giữa các ability.", "Blueprint Interface: ")

doc.add_heading("3.2 Cách làm", level=2)
add_para(
    "Kiến trúc theo mẫu Component + Object với giao tiếp qua interface, không coupling trực tiếp "
    "giữa các class. Toàn bộ tham số ability nằm trong Data Asset (không hard-code). "
    "Input đi qua Enhanced Input (Left Mouse = Fireball, Q = Heal, E = Teleport) rồi kích hoạt "
    "ability thông qua AbilitySystemComponent."
)
add_para(
    "Toàn bộ hệ thống event-driven, KHÔNG dùng Tick — sẵn sàng cho nativization và tiết kiệm bộ nhớ."
)
add_table(
    ["Chỉ số", "Giá trị"],
    [
        ["Tick usage", "0 (hoàn toàn event-driven)"],
        ["Giới hạn ability active", "Cấu hình được (mặc định 10)"],
        ["Độ mịn cooldown", "Theo từng ability và từng nhóm tag"],
        ["Cấu hình", "Toàn bộ tham số nằm trong Data Asset"],
    ],
    widths=[2.8, 4.2],
)

# ============ 4. TEST F ============
doc.add_heading("4. Test F — Enemy Creation Wizard", level=1)

doc.add_heading("4.1 Chức năng", level=2)
add_para("Editor Utility Widget 5 bước (EUW_EnemyCreationWizard) điều hướng bằng Widget Switcher:")
add_bullet("Nhập tên (bắt buộc prefix BP_), chọn thư mục output, tùy chọn tạo sub-folder.", "Bước 1 — Định danh: ")
add_bullet("Chọn Skeletal Mesh → danh sách material slot động, gán Material Interface từng slot.", "Bước 2 — Hình ảnh: ")
add_bullet("Slider Health / Base Damage / Movement Speed + Combo Box Enemy Class (Grunt/Scout/Brute/Boss).", "Bước 3 — Chỉ số: ")
add_bullet("Chọn Behavior Tree (bắt buộc).", "Bước 4 — AI: ")
add_bullet("Tóm tắt read-only + Progress Bar + Log khi sinh Blueprint.", "Bước 5 — Review & Generate: ")

doc.add_heading("4.2 Ranh giới Blueprint / C++", level=2)
add_para(
    "Wizard KHÔNG phải thuần Blueprint. Layout và tên widget nằm trong asset EUW; hành vi nằm trong "
    "plugin EnemyCreationWizardEditor. EUW được tự động reparent sang UEnemyCreationWizardWidget khi "
    "editor khởi động — nguyên tắc: designer sở hữu layout, C++ sở hữu logic. Chỉ 3 hàm phơi ra "
    "Blueprint: CreateEnemyFromWizard, OpenEnemyCreationWizard, ToggleEnemyCreationWizardOverlay."
)

# ============ 5. CÁCH LÀM PLUGIN C++ (TRỌNG TÂM) ============
doc.add_heading("5. Cách làm plugin C++ (phần trọng tâm)", level=1)

doc.add_heading("5.1 Cấu trúc 2 module", level=2)
add_para(
    "Plugin EnemyCreationWizardEditor chia làm hai module để tách bạch runtime và editor — điều "
    "kiện bắt buộc để plugin có thể đóng gói cùng game mà phần editor không bị build vào bản ship:"
)
add_table(
    ["Module", "LoadingPhase", "Vai trò"],
    [
        ["EnemyCreationWizardRuntime", "Default (Runtime)",
         "Bridge runtime → editor, OpponentComponent demo, các hàm mở/toggle wizard gọi được lúc PIE."],
        ["EnemyCreationWizardEditor", "Default (Editor)",
         "UEnemyCreationWizardWidget, generator CreateEnemyFromWizard, đăng ký menu/toolbar/hotkey, automation tests."],
    ],
    widths=[2.7, 1.5, 2.8],
)
add_para("Phụ thuộc chính trong Build.cs của module Editor:", color=GREY)
add_bullet("Public: Core, CoreUObject, Engine, UMG, Blutility, GameplayTags")
add_bullet("Private: EnemyCreationWizardRuntime, UnrealEd, AssetRegistry, AssetTools, AIModule, Slate, SlateCore, PropertyEditor, ContentBrowser, UMGEditor, InputCore, BlueprintEditorLibrary, ToolMenus, LevelEditor")
add_para(
    "Module Runtime chỉ khai báo các phụ thuộc editor (Blutility, UnrealEd, UMGEditor) trong khối "
    "if (Target.bBuildEditor) — nên khi build game thật, phần editor bị loại hoàn toàn.",
    color=GREY,
)

doc.add_heading("5.2 Tự động reparent EUW sang class C++", level=2)
add_para(
    "Khi module editor StartupModule, plugin chờ AssetRegistry load xong (qua OnFilesLoaded, hoặc "
    "FTSTicker nếu đã load), rồi load EUW blueprint và kiểm tra parent class. Nếu chưa phải "
    "UEnemyCreationWizardWidget thì:"
)
add_code(
    "WidgetBlueprint->Modify();\n"
    "UBlueprintEditorLibrary::ReparentBlueprint(WidgetBlueprint, UEnemyCreationWizardWidget::StaticClass());\n"
    "FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);\n"
    "AssetSubsystem->SaveLoadedAsset(WidgetBlueprint, false);"
)
add_para(
    "Nhờ đó designer chỉ cần chỉnh layout trong Widget Designer, còn toàn bộ hàm C++ vẫn tự gắn vào."
)

doc.add_heading("5.3 Asset picker Slate nhúng trong UMG", level=2)
add_para(
    "UMG không có widget chọn asset có lọc. Giải pháp: dựng SObjectPropertyEntryBox (Slate, từ module "
    "PropertyEditor) và nhúng vào cây widget qua UNativeWidgetHost. Các host được tạo cho: mesh, "
    "Behavior Tree, từng material slot, và path picker (FContentBrowserModule::CreatePathPicker)."
)
add_bullet("Danh sách material slot lấy từ FSkeletalMaterial::ImportedMaterialSlotName của mesh — cho phép số slot BẤT KỲ, không hard-code cứng 4 slot.")

doc.add_heading("5.4 Sinh Blueprint và ghi default bằng reflection", level=2)
add_para("Hàm CreateEnemyFromWizard chạy pipeline đầy đủ:")
add_bullet("Chuẩn hóa tên (NormalizeBlueprintName thêm/canonical prefix BP_), kiểm tra base dir, enemy class, các chỉ số Health/Damage/MovementSpeed là số hữu hạn, mesh/material/BT hợp lệ.")
add_bullet("Pre-flight: ValidateBaseEnemyContract chạy reflection trên CDO của BP_BaseEnemy TRƯỚC khi tạo gì.")
add_bullet("Tạo blueprint con qua FKismetEditorUtilities::CreateBlueprint + FAssetRegistryModule::AssetCreated + CompileBlueprint.")
add_bullet("Ghi default vào CDO bằng reflection: FindFProperty<> → SetPropertyValue_InContainer, xử lý cả FEnumProperty (Enemy Class) và FClassProperty. Blueprint không có node 'set property tùy ý theo tên trên CDO'.")
add_bullet("Ghi ~20 key provenance metadata qua UPackage::GetMetaData().SetValue; lưu qua UEditorAssetSubsystem::SaveLoadedAssets (chặn checkout).")
add_bullet("Rollback khi lỗi: DiscardUnsavedBlueprint (AssetDeleted + MarkAsGarbage).")
add_para(
    "Toàn bộ có progress modal (FScopedSlowTask) và toast báo thành công/thất bại "
    "(FSlateNotificationManager)."
)

doc.add_heading("5.5 Menu, toolbar và hotkey", level=2)
add_bullet("Đăng ký qua UToolMenus: mục trong menu Tools (LevelEditor.MainMenu.Tools) và nút toolbar (LevelEditor.LevelEditorToolBar.User).")
add_bullet("Hotkey trong PIE: FEnemyWizardPIEInputProcessor (IInputProcessor) đăng ký qua FSlateApplication::RegisterInputPreProcessor — phím K bật/tắt overlay wizard, có bỏ qua khi đang focus ô nhập text.")
add_bullet("EnsureDemoBehaviorTree tự tạo BT_TestEnemy với root Sequence nếu chưa có.")

doc.add_heading("5.6 Automation tests", level=2)
add_para(
    "4 test IMPLEMENT_SIMPLE_AUTOMATION_TEST phủ các helper thuần (không cần editor sống): "
    "NormalizeBlueprintName, ToObjectPath, ValidateMaterialSelectionCount, BuildReviewSummary. "
    "Các helper được tách riêng chính là để unit-test được."
)
add_para(
    "Nói thẳng: bản thân CreateEnemyFromWizard, các lệnh ghi CDO bằng reflection và widget cần "
    "editor sống nên chỉ verify thủ công, chưa có automation.",
    italic=True, color=GREY,
)

# ============ 6. CÁCH DEMO ============
doc.add_heading("6. Cách demo khi phỏng vấn", level=1)
add_para("Test D — Vật liệu thời tiết:")
add_bullet("Mở Lvl_ThirdPerson (đã có hàng 6 cube đeo 6 instance + BP_WeatherController).")
add_bullet("Chỉnh WetnessAmount / SnowAmount / Temperature trong MPC, hoặc set target trên controller, quan sát chuyển tiếp real-time.")
add_para("Test E — Ability:")
add_bullet("Gắn BP_AbilitySystemComponent vào nhân vật, gán DA_AbilityData, bấm Left Mouse / Q / E.")
add_para("Test F — Wizard:")
add_bullet("Mở EUW_EnemyCreationWizard, đi hết 5 bước, bấm Generate Enemy, kiểm tra Blueprint con của BP_BaseEnemy được tạo với đầy đủ mesh/material/chỉ số/BT.")

# ============ 7. CÂU HỎI THƯỜNG GẶP ============
doc.add_heading("7. Câu hỏi phỏng vấn có thể gặp & trả lời gợi ý", level=1)

qa = [
    ("Vì sao đưa thời tiết qua MPC thay vì tham số material?",
     "Để đổi thời tiết không recompile shader và không sinh material instance mới — chỉ cập nhật vài giá trị toàn cục, chi phí gần như bằng 0 và áp dụng đồng thời cho mọi bề mặt."),
    ("Vì sao Test F cần C++ mà không thuần Blueprint?",
     "Các asset picker có lọc (SObjectPropertyEntryBox), path picker, đọc tên slot material thật của mesh, tạo Blueprint và ghi default vào CDO bằng reflection, toast/progress, menu/toolbar/hotkey — đều không có node Blueprint tương đương. Blueprint vẫn giữ layout."),
    ("Ghi giá trị mặc định vào enemy mới bằng cách nào?",
     "Bằng reflection trên CDO: FindFProperty<> rồi SetPropertyValue_InContainer, xử lý cả enum và class property. Có pre-flight contract check trên BP_BaseEnemy và rollback nếu lỗi."),
    ("Hệ thống ability mở rộng thế nào?",
     "Thêm một Object Blueprint kế thừa BP_AbilityBase, override CanActivate/Execute/OnEnd, tạo Data Asset cấu hình, gán tag — không phải sửa AbilitySystemComponent."),
    ("Phần nào chưa hoàn thiện?",
     "Bản material mobile chưa có override (chưa khác desktop), UseParallax đang tắt, và chưa capture Shader Complexity. Đã ghi rõ trong tài liệu thay vì nói là đã xong."),
]
for q, a in qa:
    p = doc.add_paragraph()
    r = p.add_run("Hỏi: " + q)
    r.bold = True
    r.font.color.rgb = ACCENT
    p2 = doc.add_paragraph()
    p2.add_run("Đáp: " + a)

doc.add_paragraph()
foot = doc.add_paragraph()
foot.alignment = WD_ALIGN_PARAGRAPH.CENTER
r = foot.add_run("Xây dựng với Unreal Engine 5.8 • Technical Artist — Đào Nhật Cường")
r.italic = True
r.font.size = Pt(9)
r.font.color.rgb = GREY

out = r"E:\HocUnrealengine\TA_Test\Docs\TA_Test_Interview_Documentation.docx"
doc.save(out)
print("SAVED:", out)
