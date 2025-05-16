#include <gtk/gtk.h>

GtkWidget *input_textview, *output_textview, *tokens_view;

void on_transpile_clicked(GtkButton *button, gpointer user_data) {
    GtkTextBuffer *input_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(input_textview));
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(input_buffer, &start, &end);
    gchar *input_text = gtk_text_buffer_get_text(input_buffer, &start, &end, FALSE);

    // Dummy output — replace with your translation logic
    gchar *output = g_strdup_printf("# Translated Python Code\nprint('Hello from CodeMorph!')");

    GtkTextBuffer *output_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(output_textview));
    gtk_text_buffer_set_text(output_buffer, output, -1);

    GtkTextBuffer *tokens_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tokens_view));
    gtk_text_buffer_set_text(tokens_buffer, "TOKENS:\n[INT, MAIN, (, ), {, ...]", -1);

    g_free(input_text);
    g_free(output);
}

void on_reset_clicked(GtkButton *button, gpointer user_data) {
    gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(input_textview)), "", -1);
    gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(output_textview)), "", -1);
    gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(tokens_view)), "", -1);
}

void on_save_output_clicked(GtkButton *button, gpointer user_data) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(output_textview));
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buffer, &start, &end);
    gchar *text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

    GError *error = NULL;
    if (!g_file_set_contents("output.py", text, -1, &error)) {
        g_printerr("Error saving file: %s\n", error->message);
        g_error_free(error);
    } else {
        g_print("Output saved to output.py\n");
    }

    g_free(text);
}

GtkWidget* create_scrollable_textview(GtkWidget **textview) {
    *textview = gtk_text_view_new();
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(*textview), TRUE);
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scroll), *textview);
    gtk_widget_set_vexpand(scroll, TRUE);
    gtk_widget_set_hexpand(scroll, TRUE);
    return scroll;
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    // Apply CSS
    GtkCssProvider *css_provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(css_provider,
        "* { background-color: #1e1e2f; color: #ffffff; font-family: 'Fira Code', monospace; font-size: 14px; }"
        "textview { padding: 10px; border-radius: 5px; background-color: #2e2e3e; }"
        "button { background-color: #f472b6; color: black; font-weight: bold; padding: 6px; border-radius: 6px; border: 2px solid black; }"
        "button:hover { background-color: #ec5f98; }"
        "button:active { background-color: #d84a89; }"
        "frame { border: none; }"
        "scrolledwindow { border: none; }"
        "label { color: white; }"
        "#header-box { background-color: black; }",  // CSS for black header background
        -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(css_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "CodeMorph - C to Python");
    gtk_window_set_default_size(GTK_WINDOW(window), 1000, 600);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(window), main_box);

    // Header (black background, colored label)
    GtkWidget *header_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_name(header_box, "header-box");
    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title),
        "<span size='20000' weight='bold' foreground='#f472b6'>CodeMorph</span>  "
        "<span size='15000' foreground='white'>C → Python Transpiler</span>");
    gtk_box_pack_start(GTK_BOX(header_box), title, TRUE, TRUE, 10);
    gtk_box_pack_start(GTK_BOX(main_box), header_box, FALSE, FALSE, 0);

    // Layout: Input + Output side by side
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(main_box), hbox, TRUE, TRUE, 5);

    // ----- Left: Input area -----
    GtkWidget *left_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_pack_start(GTK_BOX(hbox), left_box, TRUE, TRUE, 5);

    // Transpile Button (above input)
    GtkWidget *transpile_btn = gtk_button_new_with_label("▶ Transpile");
    g_signal_connect(transpile_btn, "clicked", G_CALLBACK(on_transpile_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(left_box), transpile_btn, FALSE, FALSE, 5);

    // Input Frame
    GtkWidget *input_frame = gtk_frame_new(NULL);
    GtkWidget *input_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(input_label), "<b>C Code Input</b>");
    gtk_frame_set_label_widget(GTK_FRAME(input_frame), input_label);
    GtkWidget *input_scroll = create_scrollable_textview(&input_textview);
    gtk_container_add(GTK_CONTAINER(input_frame), input_scroll);
    gtk_box_pack_start(GTK_BOX(left_box), input_frame, TRUE, TRUE, 5);

    // ----- Right: Output area -----
    GtkWidget *right_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_pack_start(GTK_BOX(hbox), right_box, TRUE, TRUE, 5);

    // Reset Button (above output)
    GtkWidget *reset_btn = gtk_button_new_with_label("↺ Reset");
    g_signal_connect(reset_btn, "clicked", G_CALLBACK(on_reset_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(right_box), reset_btn, FALSE, FALSE, 5);

    // Save Output Button
    GtkWidget *save_btn = gtk_button_new_with_label("💾 Save Output");
    g_signal_connect(save_btn, "clicked", G_CALLBACK(on_save_output_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(right_box), save_btn, FALSE, FALSE, 5);

    // Notebook with Python output and tokens
    GtkWidget *notebook = gtk_notebook_new();

    GtkWidget *py_scroll = create_scrollable_textview(&output_textview);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), py_scroll, gtk_label_new("Python Output"));

    GtkWidget *tokens_scroll = create_scrollable_textview(&tokens_view);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), tokens_scroll, gtk_label_new("Tokens"));

    gtk_box_pack_start(GTK_BOX(right_box), notebook, TRUE, TRUE, 5);

    gtk_widget_show_all(window);
    gtk_main();

    return 0;
}
