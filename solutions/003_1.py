from pathlib import Path
import subprocess

from docx import Document
from docx.shared import Pt


def main():
    cwd = Path.cwd()

    dot_root = cwd / "dot"
    docx_file = cwd / "flowchart.docx"

    if not dot_root.exists():
        raise FileNotFoundError(f"未找到目录: {dot_root}")

    dot_files = sorted(
        dot_root.rglob("*.dot"),
        key=lambda p: str(p.relative_to(dot_root)).lower(),
    )

    if not dot_files:
        print("未发现 .dot 文件")
        return

    # 存在则打开，不存在则创建
    if docx_file.exists():
        doc = Document(docx_file)
    else:
        doc = Document()

    section = doc.sections[0]

    usable_width = (
        section.page_width
        - section.left_margin
        - section.right_margin
    )

    temp_dir = cwd / ".flowchart_tmp"
    temp_dir.mkdir(exist_ok=True)

    for dot_file in dot_files:

        rel_path = dot_file.relative_to(dot_root)

        png_file = (
            temp_dir
            / rel_path.with_suffix(".png")
        )

        png_file.parent.mkdir(
            parents=True,
            exist_ok=True,
        )

        print(f"生成: {rel_path}")

        subprocess.run(
            [
                "dot",
                "-Tpng",
                str(dot_file),
                "-o",
                str(png_file),
            ],
            check=True,
        )

        # 插入路径标题
        p = doc.add_paragraph()

        run = p.add_run(str(rel_path).replace("\\", "/"))

        run.bold = True
        run.font.size = Pt(12)

        # 插入图片
        doc.add_picture(
            str(png_file),
            width=usable_width,
        )

        # 图与图之间空一行
        doc.add_paragraph()

    doc.save(docx_file)

    print()
    print(f"共处理 {len(dot_files)} 个流程图")
    print(f"保存文档: {docx_file}")


if __name__ == "__main__":
    main()
