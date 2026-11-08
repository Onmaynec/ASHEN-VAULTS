# -*- coding: utf-8 -*-
"""
ASHEN VAULTS v1.0 — native Unreal Engine 4.27 material generator.
Run in UE4 Editor with PythonScriptPlugin + EditorScriptingUtilities enabled:
UE4Editor-Cmd.exe AshenVaults.uproject -ExecutePythonScript=".../Tools/Unreal/BuildVisualMaterials.py"

Creates actual .uasset master materials under /Game/AVVisuals/Materials/.
Never runs at game runtime; only editor produces the shader assets.
"""
import unreal

ROOT = "/Game/AVVisuals/Materials"
ASSETS = unreal.EditorAssetLibrary
EDIT = unreal.MaterialEditingLibrary
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()
ASSETS.make_directory(ROOT)


def new_material(name, blend):
    path = ROOT + "/" + name
    if ASSETS.does_asset_exist(path):
        material = ASSETS.load_asset(path)
        EDIT.delete_all_material_expressions(material)
    else:
        material = TOOLS.create_asset(name, ROOT, unreal.Material,
                                      unreal.MaterialFactoryNew())
    material.set_editor_property("blend_mode", blend)
    material.set_editor_property("shading_model",
                                 unreal.MaterialShadingModel.MSM_UNLIT)
    material.set_editor_property("two_sided", True)
    return material


def node(material, klass, x, y, **properties):
    obj = EDIT.create_material_expression(material, klass, x, y)
    for name, value in properties.items():
        obj.set_editor_property(name, value)
    return obj


def connect(src, output, dest, input_name):
    if not EDIT.connect_material_expressions(src, output, dest, input_name):
        raise RuntimeError("Cannot connect %s.%s -> %s.%s" %
                           (src.get_name(), output, dest.get_name(), input_name))


def finish(material, rgb, opacity=None, masked=False):
    if not EDIT.connect_material_property(rgb, "",
              unreal.MaterialProperty.MP_EMISSIVE_COLOR):
        raise RuntimeError("Cannot connect emissive output")
    if opacity:
        prop = (unreal.MaterialProperty.MP_OPACITY_MASK
                if masked else unreal.MaterialProperty.MP_OPACITY)
        if not EDIT.connect_material_property(opacity, "", prop):
            raise RuntimeError("Cannot connect opacity output")
    EDIT.recompile_material(material)
    ASSETS.save_loaded_asset(material)
    unreal.log("ASHEN VAULTS: saved %s" % material.get_path_name())


def albedo(material):
    tex = node(material, unreal.MaterialExpressionTextureSampleParameter2D,
               -700, 0, parameter_name="Albedo")
    tint = node(material, unreal.MaterialExpressionVectorParameter,
                -700, 260, parameter_name="Tint",
                default_value=unreal.LinearColor(1, 1, 1, 1))
    multiply = node(material, unreal.MaterialExpressionMultiply, -420, 80)
    connect(tex, "RGB", multiply, "A")
    connect(tint, "", multiply, "B")
    return tex, multiply


def scalar(material, name, default, x, y):
    return node(material, unreal.MaterialExpressionScalarParameter,
                x, y, parameter_name=name, default_value=default)


# Unlit pixel surface with nearest-filtered runtime atlas texture.
mat = new_material("M_AV_PixelSurface", unreal.BlendMode.BLEND_OPAQUE)
tex, color = albedo(mat)
finish(mat, color)

# Colored light: transparency from alpha, animated emissive flicker.
mat = new_material("M_AV_EmissivePulse", unreal.BlendMode.BLEND_TRANSLUCENT)
tex, color = albedo(mat)
power = scalar(mat, "GlowStrength", 2.2, -420, 260)
time = node(mat, unreal.MaterialExpressionTime, -700, 460)
speed = scalar(mat, "PulseSpeed", 4.0, -700, 550)
multiply_time = node(mat, unreal.MaterialExpressionMultiply, -480, 440)
connect(time, "", multiply_time, "A")
connect(speed, "", multiply_time, "B")
sine = node(mat, unreal.MaterialExpressionSine, -290, 450)
connect(multiply_time, "", sine, "Input")
pulse = node(mat, unreal.MaterialExpressionMultiply, -100, 450)
connect(sine, "", pulse, "A")
half = scalar(mat, "PulseAmplitude", 0.22, -300, 565)
connect(half, "", pulse, "B")
offset = node(mat, unreal.MaterialExpressionAdd, 30, 410)
connect(pulse, "", offset, "A")
one = scalar(mat, "PulseBase", 1.0, 10, 550)
connect(one, "", offset, "B")
base_glow = node(mat, unreal.MaterialExpressionMultiply, -70, 100)
connect(color, "", base_glow, "A")
connect(power, "", base_glow, "B")
final_glow = node(mat, unreal.MaterialExpressionMultiply, 220, 130)
connect(base_glow, "", final_glow, "A")
connect(offset, "", final_glow, "B")
finish(mat, final_glow, tex, False)

# Damage flash: heavy color tint with the image's alpha.
mat = new_material("M_AV_HitFlash", unreal.BlendMode.BLEND_TRANSLUCENT)
tex, color = albedo(mat)
flash = node(mat, unreal.MaterialExpressionVectorParameter, -400, 250,
             parameter_name="FlashColor",
             default_value=unreal.LinearColor(1.0, 0.22, 0.15, 1.0))
mix = node(mat, unreal.MaterialExpressionMultiply, -190, 50)
connect(color, "", mix, "A")
connect(flash, "", mix, "B")
finish(mat, mix, tex, False)

# Masked dissolve for phase shifts and corpse disintegration. Procedural noise
# is rebuilt as an editor asset; runtime may animate DissolveAmount on an MID.
mat = new_material("M_AV_Dissolve", unreal.BlendMode.BLEND_MASKED)
tex, color = albedo(mat)
noise = node(mat, unreal.MaterialExpressionNoise, -540, 330)
uv = node(mat, unreal.MaterialExpressionTextureCoordinate, -740, 330)
connect(uv, "", noise, "Position")
threshold = scalar(mat, "DissolveAmount", 0.0, -350, 470)
branch = node(mat, unreal.MaterialExpressionIf, -30, 330)
connect(noise, "", branch, "A")
connect(threshold, "", branch, "B")
yes = node(mat, unreal.MaterialExpressionConstant, -260, 560, r=1.0)
no = node(mat, unreal.MaterialExpressionConstant, -260, 630, r=0.0)
connect(yes, "", branch, "A>B")
connect(yes, "", branch, "A==B")
connect(no, "", branch, "A<B")
alpha = node(mat, unreal.MaterialExpressionMultiply, 175, 270)
connect(tex, "A", alpha, "A")
connect(branch, "", alpha, "B")
finish(mat, color, alpha, True)

unreal.log("ASHEN VAULTS: all v1.0.0 material graphs generated.")
