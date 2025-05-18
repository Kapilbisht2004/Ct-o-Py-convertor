import sys
import subprocess
from PyQt5.QtWidgets import (
    QApplication, QWidget, QLabel, QVBoxLayout, QHBoxLayout,
    QPushButton, QPlainTextEdit, QFileDialog, QTabWidget, QMessageBox,
    QSizePolicy, QSpacerItem, QStyle
)
from PyQt5.QtGui import QFont
from PyQt5.QtCore import Qt


class Transpiler(QWidget):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("CodeMorph → C2🐍")
        self.setMinimumSize(1500, 800)
        self.setup_ui()

    def setup_ui(self):
        layout = QVBoxLayout(self)
        layout.setContentsMargins(20, 20, 20, 20)

        # Header Section
        header_container = QVBoxLayout()
        header_container.setSpacing(5)
        header = QLabel("CodeMorph")
        header.setStyleSheet("color: #f472b6; font-size: 40px; font-weight: 900;")
        header.setAlignment(Qt.AlignCenter)
        subheader = QLabel("C → Python")
        subheader.setStyleSheet("color: #cbd5e1; font-size: 18px; font-weight: 600; margin-bottom: 15px;")
        subheader.setAlignment(Qt.AlignCenter)
        header_container.addWidget(header)
        header_container.addWidget(subheader)
        layout.addLayout(header_container)

        main_split = QHBoxLayout()
        layout.addLayout(main_split)

        # Create the main buttons layout that will span both panels
        action_buttons_layout = QHBoxLayout()
        action_buttons_layout.setSpacing(10)

        # Left side buttons
        self.open_file_btn = QPushButton(" Open File")
        self.open_file_btn.setIcon(self.style().standardIcon(QStyle.SP_DialogOpenButton))
        self.open_file_btn.setFixedHeight(40)
        self.open_file_btn.setCursor(Qt.PointingHandCursor)
        self.open_file_btn.setToolTip("Open a C source file (.c)")
        self.open_file_btn.clicked.connect(self.open_file)

        self.transpile_btn = QPushButton(" Transpile")
        self.transpile_btn.setIcon(self.style().standardIcon(QStyle.SP_MediaPlay))
        self.transpile_btn.setFixedHeight(40)
        self.transpile_btn.setCursor(Qt.PointingHandCursor)
        self.transpile_btn.setToolTip("Transpile the C code to Python")
        self.transpile_btn.clicked.connect(self.transpile_code)

        self.reset_btn = QPushButton(" Reset")
        self.reset_btn.setIcon(self.style().standardIcon(QStyle.SP_DialogResetButton))
        self.reset_btn.setFixedHeight(40)
        self.reset_btn.setCursor(Qt.PointingHandCursor)
        self.reset_btn.setToolTip("Clear all input and output fields")
        self.reset_btn.clicked.connect(self.reset_fields)

        # Add left side buttons to the layout
        action_buttons_layout.addWidget(self.open_file_btn)
        action_buttons_layout.addWidget(self.transpile_btn)
        action_buttons_layout.addWidget(self.reset_btn)

        # Add a spacer to push the save button to the right side
        action_buttons_layout.addItem(QSpacerItem(40, 20, QSizePolicy.Expanding, QSizePolicy.Minimum))

        # Right side save button
        self.save_btn = QPushButton(" Save Output")
        self.save_btn.setIcon(self.style().standardIcon(QStyle.SP_DialogSaveButton))
        self.save_btn.setFixedHeight(40)
        self.save_btn.setCursor(Qt.PointingHandCursor)
        self.save_btn.setToolTip("Save the Python output to a file")
        self.save_btn.clicked.connect(self.save_output)
        action_buttons_layout.addWidget(self.save_btn)

        # Add the common buttons layout to the main layout before the split
        layout.addLayout(action_buttons_layout)

        # Now create the split for the input and output areas
        left_panel = QVBoxLayout()
        left_panel.setSpacing(10)

        self.input_box = QPlainTextEdit()
        self.input_box.setFont(QFont("Fira Code", 12))
        self.input_box.setPlaceholderText("Enter C code here or click 'Open File'...")
        left_panel.addWidget(self.input_box)

        main_split.addLayout(left_panel, 3)

        # Right side: Tabs with output
        right_panel = QVBoxLayout()
        right_panel.setSpacing(10)

        # Tabs for output
        self.tabs = QTabWidget()
        self.tabs.setStyleSheet("""
            QTabBar::tab {
                background: #2e2e3e;
                color: #cbd5e1;
                padding: 10px 20px; /* This padding affects tab height */
                font-weight: 600;
                border-top-left-radius: 6px;
                border-top-right-radius: 6px;
                margin-right: 2px; /* Spacing between tabs */
            }
            QTabBar::tab:selected {
                background: #f472b6;
                color: white;
            }
            QTabBar::tab:hover {
                background: #3a3a4f;
                color: #e0e0e0;
            }
        """)

        self.output_box = QPlainTextEdit()
        self.output_box.setFont(QFont("Fira Code", 12))
        self.output_box.setReadOnly(True)
        self.output_box.setPlaceholderText("Transpiled Python code will appear here.")

        self.tokens_box = QPlainTextEdit()
        self.tokens_box.setFont(QFont("Fira Code", 12))
        self.tokens_box.setReadOnly(True)
        self.tokens_box.setPlaceholderText("Lexical tokens will appear here.")

        self.ast_box = QPlainTextEdit()
        self.ast_box.setFont(QFont("Fira Code", 12))
        self.ast_box.setReadOnly(True)
        self.ast_box.setPlaceholderText("Abstract Syntax Tree (AST) representation will appear here.")

        self.tabs.addTab(self.output_box, ".PY code")
        self.tabs.addTab(self.tokens_box, "Tokens")
        self.tabs.addTab(self.ast_box, "AST")

        right_panel.addWidget(self.tabs)
        main_split.addLayout(right_panel, 4)

        # Global stylesheet
        self.setStyleSheet("""
            QWidget {
                background-color: #1e1e2f;
                color: #cbd5e1;
                font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            }
            QPushButton {
                background-color: #f472b6;
                border: none;
                border-radius: 8px;
                font-weight: 700;
                font-size: 14px;
                color: white;
                padding: 0px 18px; /* Default padding for most buttons */
            }
            QPushButton:hover {
                background-color: #ec5f98;
            }
            QPushButton:pressed {
                background-color: #d84a89;
            }
            QPlainTextEdit {
                background-color: #2e2e3e;
                border-radius: 8px;
                padding: 12px;
                border: 1px solid #44475a;
                color: #e0e0e0;
                font-size: 18px;
            }
            QTabWidget::pane { /* Styles the area *behind* the tabs content */
                border-top: 2px solid #f472b6;
                border-radius: 0 0 8px 8px; /* Only bottom corners rounded */
            }
        """)

    def open_file(self):
        options = QFileDialog.Options()
        file_path, _ = QFileDialog.getOpenFileName(
            self,
            "Open C File",
            "",
            "C Files (*.c);;All Files (*.c)",
            options=options
        )
        if file_path:
            try:
                with open(file_path, 'r', encoding='utf-8') as f:
                    content = f.read()
                self.input_box.setPlainText(content)
                self.output_box.clear()
                self.tokens_box.clear()
                self.ast_box.clear()
                self.tabs.setCurrentIndex(0)
            except Exception as e:
                QMessageBox.critical(self, "Error", f"Could not read file: {str(e)}")

    def transpile_code(self):
        input_text = self.input_box.toPlainText()
        if not input_text.strip():
            QMessageBox.warning(self, "Warning", "Input C code is empty. Please enter or open C code.")
            return

        self.output_box.clear()
        self.tokens_box.clear()
        self.ast_box.clear()
        self.tabs.setCurrentIndex(0)

        try:
            creation_flags = 0
            if sys.platform == 'win32':
                creation_flags = subprocess.CREATE_NO_WINDOW

            process = subprocess.Popen(
                ['transpiler.exe'],
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                encoding='utf-8',
                creationflags=creation_flags
            )
            stdout, stderr = process.communicate(input=input_text)

            if process.returncode != 0 or (stderr and "---PYTHON_CODE---" not in stdout):
                error_message = "Transpiler Error:\n"
                if stderr: error_message += stderr.strip()
                elif stdout and "---PYTHON_CODE---" not in stdout: error_message += stdout.strip()
                else: error_message += "Unknown error during transpilation."
                QMessageBox.critical(self, "Transpilation Error", error_message)
                return

            tokens, ast_content, python_code = "", "", ""
            section = None
            for line in stdout.splitlines():
                if line.strip() == "---TOKENS---": section = "tokens"; continue
                elif line.strip() == "---AST---": section = "ast"; continue
                elif line.strip() == "---PYTHON_CODE---": section = "python"; continue

                if section == "tokens": tokens += line + "\n"
                elif section == "ast": ast_content += line + "\n"
                elif section == "python": python_code += line + "\n"

            if not python_code.strip() and not tokens.strip() and not ast_content.strip() and not stderr:
                 QMessageBox.warning(self, "Transpilation Note", "Transpiler produced no distinct output sections.")

            self.tokens_box.setPlainText(tokens.strip())
            self.ast_box.setPlainText(ast_content.strip())
            self.output_box.setPlainText(python_code.strip())
            
            if stderr.strip() and "---PYTHON_CODE---" in stdout: # Check if python code exists despite stderr
                QMessageBox.information(self, "Transpiler Messages", f"Transpilation completed with messages:\n\n{stderr.strip()}")

        except FileNotFoundError:
            QMessageBox.critical(self, "Error", "'transpiler.exe' not found. Ensure it is in PATH or same directory as the script.")
        except Exception as e:
            QMessageBox.critical(self, "Error", f"An unexpected error occurred: {str(e)}")

    def reset_fields(self):
        self.input_box.clear()
        self.output_box.clear()
        self.tokens_box.clear()
        self.ast_box.clear()
        self.tabs.setCurrentIndex(0)

    def save_output(self):
        python_code = self.output_box.toPlainText()
        if not python_code.strip():
            QMessageBox.warning(self, "Nothing to Save", "The Python output is empty.")
            return

        path, _ = QFileDialog.getSaveFileName(self, "Save Python Output", "Converted.py", "Python Files (*.py);;All Files (*)")
        if path:
            try:
                with open(path, "w", encoding='utf-8') as f:
                    f.write(python_code)
                QMessageBox.information(self, "Success", f"Output successfully saved to {path}")
            except Exception as e:
                QMessageBox.critical(self, "Error", f"Could not save file: {str(e)}")


if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = Transpiler()
    window.show()
    sys.exit(app.exec_())