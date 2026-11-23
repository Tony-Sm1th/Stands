# DDCViewer_optimized.py
import tkinter as tk
from tkinter import ttk, messagebox
from tkinter.scrolledtext import ScrolledText
import serial
import serial.tools.list_ports
from serial.serialutil import SerialException, SerialTimeoutException
import threading
import queue
from matplotlib.figure import Figure
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import numpy as np
import base64
from PIL import Image, ImageTk
import os
import time
from datetime import datetime
from functools import wraps
import struct
import io
import sys
# ========== КОНСТАНТЫ И КОНФИГУРАЦИЯ ==========

class AppConfig:
    """Конфигурация приложения"""
    ICONS = {
        "app_icon": "iVBORw0KGgoAAAANSUhEUgAAAEAAAABACAYAAACqaXHeAAAACXBIWXMAAA7DAAAOwwHHb6hkAAAAGXRFWHRTb2Z0d2FyZQB3d3cuaW5rc2NhcGUub3Jnm+48GgAAAuhJREFUeJztmU9IFHEUxz9vVrf1T0RFhnYQ9FZhEXSJuoQEXbSIpVMgBZ4VCrZOc9OKDhEdkqBD0CEPpUEXoU4dAhWM7JBIp0gxMkNSd3bmddAd/4S6xjS/hfl9TvN29vfed7483v7mt2CxWCwWiyWpSFyFLub0bAAnAUT4fnwXT1xXgrjqb0ZFPGVUFAYFapZDGFvkC/Amnvqb48RRxHURig+/gi/UxlF7O2IxoJyxBpgWYBprgGkBpkm8ASXvA9puaUsq4KlCLYKK8uhFr9z918KpIJ5NWDarKa+ZEeAYgMB4RYYT/a7kYQcdIAHnFVqAJpTmAK7+H8nRUmhkPysPD6BwxPeoL8Yld4BsMEsi3EZnu7XKS/MAoXEl+dDLHrkTVf6tKIsZ4FVxCuEa0Aq0ovRmXU3HUduYAb6DFq/V/6sThTlScegoiw4wiTXAtADTxHQe8Dc72Qe4rjqjC5wWIS0OWpnmfb8r81HoMGbAThhbossR7gEQgLfEc+ByFLnL4ldgO1Rp2BAfikpH4mdA4g2IbAZ0dmrl1D6OOuCokh+4zUeQTds8ypehDlczv/IcDgJEhYWBHvlU6trIOmB6Lw8dGAWGRfjQnqMrqtzbMbfIsyBgBBgWZfzCTb1S6trIDBBo2ireyE6G4HboxlpKc6lrEz8DEm/AuiF47rrW7K7gIEA+w9QrV36bkRUfYQdku7WquoIJHyZ9mEwtMtHhasakuDgIDfCrqYPVoyKgYdbjQPyS4iXxM8AaYFqAaawBpgWYxhpgWoBprAGmBZjGGmBagGmsAaYFmMYaYFqAaUIDvEJ0h5TlztpnDQ3IVDEFTBtRFC/T9bN8KwbhmWC/K/lLN/SMn6JNVz5Xj5/F+07AW98hF8bCj7VZBR4HMLTm+6PFa9dF23OrawFQwj8vgko+O4X192f24K3J/TqAmTBWvq5LJdxHqQtrK++K14Va5p3F5dwChZTPYF+feFgsFovFYkk8fwBTJsUPO80N8wAAAABJRU5ErkJggg==",
        "parrot": "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"
    }
    
    CAPACITOR_OPTIONS = {
        '12.5 пКл': 0x0,
        '50 пКл': 0x02,
        '100 пКл': 0x04,
        '150 пКл': 0x06
    }
    
    CAPACITOR_VALUES_PC = {
        '12.5 пКл': 12.5,
        '50 пКл': 50.0,
        '100 пКл': 100.0,
        '150 пКл': 150.0
    }
    
    TRIGGER_OPTIONS = ['Аппаратный', 'Программный']
    BAUD_RATES = ['9600', '19200', '38400', '57600', '115200', '2000000']
    
    COMMAND_CODES = {
        'config': 0x05,
        'accumulate': 0x15,
        'send': 0x45
    }
    
    DEFAULTS = {
        'baud': '115200',
        'conversion_interval': '1000',
        'averaging_cycles': '20',
        'auto_interval': '1000',
        'trigger_level': '500',
        'capacitor': '50 пКл',
        'trigger': 'Аппаратный'
    }
    
    CHECK_WORD_EXPECTED = 0xAAAA




# ========== ДЕКОРАТОРЫ ДЛЯ ОБРАБОТКИ ОШИБОК ==========

def handle_serial_errors(func):
    """Декоратор для обработки ошибок последовательного порта"""
    @wraps(func)
    def wrapper(self, *args, **kwargs):
        if not self.is_connected:
            self.log_message("✗ Не подключено к COM порту")
            return None
        try:
            return func(self, *args, **kwargs)
        except SerialException as e:
            self.log_message(f"✗ Ошибка COM порта: {e}")
            if self.is_connected:
                self.root.after(0, self.disconnect_serial)
        except SerialTimeoutException:
            self.log_message("✗ Таймаут отправки команды")
        except Exception as e:
            self.log_message(f"✗ Ошибка в {func.__name__}: {e}")
        return None
    return wrapper


def validate_connection(func):
    """Декоратор для проверки подключения"""
    @wraps(func)
    def wrapper(self, *args, **kwargs):
        if not self.is_connected:
            messagebox.showwarning("Не подключено", "Сначала подключитесь к COM порту!")
            return None
        return func(self, *args, **kwargs)
    return wrapper


# ========== ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ==========

class Utils:
    """Вспомогательные утилиты"""
    
    @staticmethod
    def create_photoimage_from_base64(base64_string):
        """Создать PhotoImage из base64 строки"""
        try:
            icon_data = base64.b64decode(base64_string)
            image = Image.open(io.BytesIO(icon_data))
            return ImageTk.PhotoImage(image)
        except Exception as e:
            print(f"Ошибка при создании изображения: {e}")
            return None
    
    @staticmethod
    def create_matplotlib_image_from_base64(base64_string):
        """Создать изображение для matplotlib из base64 строки"""
        try:
            icon_data = base64.b64decode(base64_string)
            image = Image.open(io.BytesIO(icon_data))
            if image.mode != 'RGBA':
                image = image.convert('RGBA')
            return np.array(image)
        except Exception as e:
            print(f"Ошибка при создании изображения: {e}")
            return None
    
    @staticmethod
    def get_timestamp():
        """Получение временной метки для лога"""
        return datetime.now().strftime("[%H:%M:%S]")
    
    @staticmethod
    def combine_words_to_float(word1, word2):
        """Объединяет два 16-битных слова в одно 32-битное float"""
        try:
            bytes_data = struct.pack('<HH', word1, word2)
            return struct.unpack('<f', bytes_data)[0]
        except Exception as e:
            raise ValueError(f"Ошибка преобразования в float: {e}")


# ========== ОСНОВНОЙ КЛАСС ПРИЛОЖЕНИЯ ==========

class SerialBarPlotApp:
    def __init__(self, root):
        self.root = root
        self.root.title("DDCViewer")
        # self.root.geometry("800x600")
        self.root.state('zoomed')
        # Добавьте минимальные размеры окна
        self.root.minsize(800, 600)  # Минимальная ширина и высота
        
        # Инициализация конфигурации
        self.config = AppConfig()
        self.utils = Utils()
        # Настройка стилей
        self.setup_styles()

        # Инициализация иконки
        self.setup_icon()
        
        # Инициализация переменных
        self.init_variables()
        
        # Создание интерфейса
        self.create_widgets()
        self.setup_plot()
        
        # Запуск потока для обновления данных
        self.update_data()

        # # Привязка события изменения размера окна
        # self.root.bind("<Configure>", self.on_window_resize)
    
        # Обработчик закрытия окна
        self.root.protocol("WM_DELETE_WINDOW", self.on_closing)

    # def on_window_resize(self, event):
    #     """Обработка изменения размера окна"""
    #     # Небольшая задержка для стабилизации размера
    #     self.root.after(100, self.update_layout)

    def setup_styles(self):
        """Настройка стилей виджетов"""
        style = ttk.Style()
        
        # Стиль для активного разделителя
        style.configure('Trough.Horizontal.TSeparator', 
                    troughcolor='#4a90e2', 
                    background='#4a90e2')
        
    def init_variables(self):
        """Инициализация переменных"""
        # Serial port variables
        self.serial_port = None
        self.is_connected = False
        self.data_queue = queue.Queue()
        self.data_buffer = np.zeros(128)
        self.center_of_gravity = 0.0
        self.extra_value = None
        
        # Command parameters
        self.command_params = [0] * 10
        self.is_auto_reading = False
        self.auto_timer = None
        
        # Widget references storage
        self.widgets = {}
        
        # Log file variables
        self.log_enabled = False
        self.log_file = None
        self.log_filename = "log.txt"
        


        # Добавляем инициализацию высоты панели управления
        self.commands_height = 280
        self.is_resizing = False

        # Для обработки текстовых сообщений
        self.text_buffer = ""
        self.binary_buffer = bytearray()
        self.last_command = None  # 'config', 'send', 'accumulate'

    def setup_icon(self):
        """Установка иконки приложения"""
        icon = self.utils.create_photoimage_from_base64(self.config.ICONS["app_icon"])
        if icon:
            self.root.iconphoto(True, icon)

    # ========== ГРАФИЧЕСКИЙ ИНТЕРФЕЙС ==========
    # (Все методы создания интерфейса остаются без изменений)
    
    def create_widgets(self):
        """Создание всех виджетов интерфейса"""
        self.tab_control = ttk.Notebook(self.root)
        self.tab_control.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # Основные вкладки
        tabs = [
            ("Управление и график", self.create_main_tab_content),
            ("Лог и команды", self.create_log_tab_content)
        ]
        
        for tab_name, create_func in tabs:
            tab_frame = ttk.Frame(self.tab_control)
            self.tab_control.add(tab_frame, text=tab_name)
            create_func(tab_frame)
        
        # Initial refresh
        self.refresh_ports()

    def create_main_tab_content(self, parent):
        """Создание содержимого основной вкладки"""
        # Фреймы
        self.create_commands_frame(parent)
        # Добавьте разделитель с возможностью изменения размера
        self.create_resizable_separator(parent)
        self.create_plot_frame(parent)
        self.setup_main_layout(parent)

    def create_commands_frame(self, parent):
        """Создание фрейма команд управления"""
        commands_frame = ttk.LabelFrame(parent, text="Панель управления", 
                                        padding="10", takefocus=0)
        commands_frame.grid(row=1, column=0, columnspan=3, 
                        sticky=(tk.W, tk.E), padx=5, pady=0)
        commands_frame.grid_propagate(False)
        commands_frame.config(height=self.commands_height)  # Используем переменную высоты
        
        # Сохраняем ссылку
        self.widgets['commands_frame'] = commands_frame
        
        self.create_scrollable_frame(commands_frame)

    def create_resizable_separator(self, parent):
        """Создание перемещаемого разделителя"""
        # Создаем фрейм для разделителя
        separator_frame = ttk.Frame(parent, height=8, takefocus=0)
        separator_frame.grid(row=2, column=0, columnspan=3, sticky=(tk.W, tk.E), padx=5, pady=2)
        separator_frame.grid_propagate(False)
        
        # Создаем сам разделитель
        self.separator = ttk.Separator(separator_frame, orient='horizontal')
        self.separator.pack(fill=tk.X, expand=True)
        
        # Изменяем курсор при наведении на разделитель
        self.separator.bind("<Enter>", lambda e: separator_frame.config(cursor="sizing"))
        self.separator.bind("<Leave>", lambda e: separator_frame.config(cursor=""))
        
        # Обработчики для перетаскивания
        self.separator.bind("<ButtonPress-1>", self.start_resize)
        self.separator.bind("<B1-Motion>", self.do_resize)
        self.separator.bind("<ButtonRelease-1>", self.stop_resize)
        
        # Сохраняем ссылки
        self.widgets['separator_frame'] = separator_frame
        

    def create_scrollable_frame(self, parent):
        """Создание прокручиваемой области для команд"""
        # Создаем фрейм с двойной прокруткой
        container = ttk.Frame(parent, takefocus=0)
        container.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        # Настраиваем сетку для растяжения
        parent.columnconfigure(0, weight=1)
        parent.rowconfigure(0, weight=1)
        container.columnconfigure(0, weight=1)
        container.rowconfigure(0, weight=1)
        
        # Создаем канвас с прокрутками
        canvas = tk.Canvas(container, highlightthickness=0, takefocus=0)
        canvas.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        # Вертикальная прокрутка
        v_scrollbar = ttk.Scrollbar(container, orient="vertical", command=canvas.yview)
        v_scrollbar.grid(row=0, column=1, sticky=(tk.N, tk.S))
        
        # Горизонтальная прокрутка
        h_scrollbar = ttk.Scrollbar(parent, orient="horizontal", command=canvas.xview)
        h_scrollbar.grid(row=1, column=0, sticky=(tk.W, tk.E))
        
        canvas.configure(yscrollcommand=v_scrollbar.set, xscrollcommand=h_scrollbar.set)
        
        inner_frame = ttk.Frame(canvas, takefocus=0)
        canvas.create_window((0, 0), window=inner_frame, anchor="nw")
        
        # Создаем виджеты в колонках
        self.create_command_columns(inner_frame)
        
        # Сохраняем ссылки
        self.widgets['commands_canvas'] = canvas
        self.widgets['commands_inner_frame'] = inner_frame
        self.widgets['v_scrollbar'] = v_scrollbar
        self.widgets['h_scrollbar'] = h_scrollbar
        
        # Обновляем размеры
        inner_frame.update_idletasks()
        
        def configure_canvas(event):
            # Устанавливаем область прокрутки
            canvas.config(scrollregion=canvas.bbox("all"))
            # Устанавливаем минимальную ширину внутреннего фрейма равной ширине канваса
            canvas.itemconfig(1, width=max(canvas.winfo_width(), inner_frame.winfo_reqwidth()))
        
        inner_frame.bind("<Configure>", configure_canvas)
        canvas.bind("<Configure>", configure_canvas)
        
        # Включаем прокрутку колесом мыши
        def _on_mousewheel(event):
            canvas.yview_scroll(int(-1*(event.delta/120)), "units")
        
        canvas.bind_all("<MouseWheel>", _on_mousewheel)

    def create_command_columns(self, parent):
        """Создание колонок с командами"""
        column_configs = [
            ("Управление подключением", self.create_connection_column),
            ("Конверсия и усреднение", self.create_conversion_column),
            ("Триггер", self.create_trigger_column),
            ("Фон", self.create_background_column),
            ("Команды", self.create_commands_column),
            ("Центр тяжести и запись", self.create_gravity_column),
            ("Выбранный канал", self.create_selected_channel_column)
        ]
        
        # Используем grid для размещения колонок в строке
        for i, (title, create_func) in enumerate(column_configs):
            col_frame = ttk.LabelFrame(parent, text=title, padding="5", takefocus=0)
            col_frame.grid(row=0, column=i, sticky=(tk.W, tk.E, tk.N, tk.S), 
                        padx=(0, 10) if i < len(column_configs)-1 else 0)
            create_func(col_frame)
            
            # Настройка растяжения внутри колонки
            col_frame.columnconfigure(0, weight=1)
        
        # Настройка растяжения родительского фрейма
        parent.columnconfigure("all", weight=0)  # Не растягивать колонки
        parent.rowconfigure(0, weight=1)


    def create_selected_channel_column(self, parent):
        """Колонка для отображения выбранного канала и общего тока"""
        # Заголовок
        title_label = ttk.Label(parent, text="Параметры канала", 
                                font=('Arial', 11, 'bold'))
        title_label.grid(row=0, column=0, columnspan=2, sticky=tk.W, pady=(0, 10))
        
        # Channel number
        ttk.Label(parent, text="Канал:", font=('Arial', 10)).grid(
            row=1, column=0, sticky=tk.W, pady=(0, 5))
        
        self.widgets['selected_channel'] = tk.Label(
            parent, 
            text="---", 
            font=('Arial', 12, 'bold'),
            foreground="green",
            width=6,
            anchor='w'
        )
        self.widgets['selected_channel'].grid(row=1, column=1, sticky=tk.W, pady=(0, 5), padx=(5, 0))
        
        # Relative value
        ttk.Label(parent, text="Значение:", font=('Arial', 10)).grid(
            row=2, column=0, sticky=tk.W, pady=(0, 5))
        
        self.widgets['selected_value'] = tk.Label(
            parent, 
            text="---", 
            font=('Arial', 12, 'bold'),
            foreground="blue",
            width=6,
            anchor='w'
        )
        self.widgets['selected_value'].grid(row=2, column=1, sticky=tk.W, pady=(0, 5), padx=(5, 0))
        
        # Current value for selected channel
        ttk.Label(parent, text="Ток канала:", font=('Arial', 10)).grid(
            row=3, column=0, sticky=tk.W, pady=(0, 5))
        
        self.widgets['selected_current'] = tk.Label(
            parent, 
            text="--- нА", 
            font=('Arial', 11, 'bold'),
            foreground="red",
            width=15,
            anchor='w'
        )
        self.widgets['selected_current'].grid(row=3, column=1, sticky=tk.W, pady=(0, 5), padx=(5, 0))
        
        # Разделитель
        separator = ttk.Separator(parent, orient='horizontal')
        separator.grid(row=4, column=0, columnspan=2, sticky=(tk.W, tk.E), pady=10)
        
        # Total current
        ttk.Label(parent, text="Общий ток:", font=('Arial', 10, 'bold')).grid(
            row=5, column=0, sticky=tk.W, pady=(0, 5))
        
        self.widgets['total_current'] = tk.Label(
            parent, 
            text="--- нА", 
            font=('Arial', 13, 'bold'),
            foreground="darkred",
            width=15,
            anchor='w'
        )
        self.widgets['total_current'].grid(row=5, column=1, sticky=tk.W, pady=(0, 5), padx=(5, 0))
        
        # Настройка веса колонок для выравнивания по левому краю
        parent.columnconfigure(0, weight=0)
        parent.columnconfigure(1, weight=1)
        
        # Установка минимальной ширины для выравнивания
        parent.update_idletasks()
    def create_connection_column(self, parent):
        """Колонка управления подключением"""
        # Port selection
        ttk.Label(parent, text="COM порт:").grid(row=0, column=0, sticky=tk.W, pady=(0, 5))
        self.widgets['port_combo'] = ttk.Combobox(parent, width=20, takefocus=0)
        self.widgets['port_combo'].grid(row=1, column=0, sticky=tk.W, pady=(0, 10))
        
        ttk.Button(parent, text="🔄 Обновить", 
                  command=self.refresh_ports, width=12).grid(row=2, column=0, sticky=tk.W, pady=(0, 10))
        
        # Baud rate
        ttk.Label(parent, text="Скорость:").grid(row=3, column=0, sticky=tk.W, pady=(0, 5))
        self.widgets['baud_combo'] = ttk.Combobox(parent, 
                                                 values=self.config.BAUD_RATES,
                                                 width=10, state="readonly")
        self.widgets['baud_combo'].grid(row=4, column=0, sticky=tk.W, pady=(0, 10))
        self.widgets['baud_combo'].set(self.config.DEFAULTS['baud'])
        
        # Connection button
        self.widgets['connect_btn'] = ttk.Button(parent, text="Подключиться", 
                                                command=self.toggle_connection, width=15)
        self.widgets['connect_btn'].grid(row=5, column=0, sticky=tk.W, pady=(0, 10))
        
        # Status indicator
        self.widgets['status_label'] = ttk.Label(parent, text="Отключено", 
                                                foreground="red", font=('Arial', 10, 'bold'))
        self.widgets['status_label'].grid(row=6, column=0, sticky=tk.W, pady=(0, 5))

    def create_conversion_column(self, parent):
        """Колонка конверсии и усреднения"""
        widgets = [
            ("Интервал конверсии (мкс):", 'conv_entry', self.config.DEFAULTS['conversion_interval']),
            ("Усреднение:", 'avg_var', None),  # Radio buttons
            ("Циклов усреднения:", 'it_entry', self.config.DEFAULTS['averaging_cycles'])
        ]
        
        for i, (label, key, default) in enumerate(widgets):
            row = i * 2
            ttk.Label(parent, text=label).grid(row=row, column=0, sticky=tk.W, pady=(0, 5))
            
            if key == 'avg_var':
                avg_frame = ttk.Frame(parent, takefocus=0)
                avg_frame.grid(row=row+1, column=0, sticky=tk.W, pady=(0, 10))
                self.widgets[key] = tk.IntVar(value=0)
                ttk.Radiobutton(avg_frame, text="Вкл", variable=self.widgets[key], value=1).pack(side=tk.LEFT)
                ttk.Radiobutton(avg_frame, text="Выкл", variable=self.widgets[key], value=0).pack(side=tk.LEFT, padx=(10, 0))
            else:
                self.widgets[key] = ttk.Entry(parent, width=15)
                self.widgets[key].grid(row=row+1, column=0, sticky=tk.W, pady=(0, 10 if i < len(widgets)-1 else 5))
                if default:
                    self.widgets[key].insert(0, default)

    def create_trigger_column(self, parent):
        """Колонка триггера"""
        # Trigger mode
        ttk.Label(parent, text="Режим триггера:").grid(row=0, column=0, sticky=tk.W, pady=(0, 5))
        self.widgets['trigger_combo'] = ttk.Combobox(parent, values=self.config.TRIGGER_OPTIONS,
                                                    width=15, state="readonly")
        self.widgets['trigger_combo'].grid(row=1, column=0, sticky=tk.W, pady=(0, 10))
        self.widgets['trigger_combo'].set(self.config.DEFAULTS['trigger'])
        
        # Pulse mode
        self.widgets['pulse_var'] = tk.IntVar(value=0)
        ttk.Checkbutton(parent, text="Импульсный режим", 
                       variable=self.widgets['pulse_var']).grid(row=2, column=0, sticky=tk.W, pady=(0, 10))
        
        # Trigger level
        ttk.Label(parent, text="Уровень:").grid(row=3, column=0, sticky=tk.W, pady=(0, 5))
        self.widgets['trigger_entry'] = ttk.Entry(parent, width=15)
        self.widgets['trigger_entry'].grid(row=4, column=0, sticky=tk.W, pady=(0, 5))
        self.widgets['trigger_entry'].insert(0, self.config.DEFAULTS['trigger_level'])

    def create_background_column(self, parent):
        """Колонка фона"""
        # Background accumulation
        ttk.Label(parent, text="Фон:").grid(row=0, column=0, sticky=tk.W, pady=(0, 5))
        ttk.Button(parent, text="Накопить", width=12,
                  command=lambda: self.send_simple_command(self.config.COMMAND_CODES['accumulate']))\
                  .grid(row=1, column=0, sticky=tk.W, pady=(0, 10))
        
        # Background subtraction
        ttk.Label(parent, text="Вычитание:").grid(row=2, column=0, sticky=tk.W, pady=(0, 5))
        bg_frame = ttk.Frame(parent)
        bg_frame.grid(row=3, column=0, sticky=tk.W, pady=(0, 5))
        self.widgets['noise_var'] = tk.IntVar(value=0)
        ttk.Radiobutton(bg_frame, text="Вкл", variable=self.widgets['noise_var'], value=1).pack(side=tk.LEFT)
        ttk.Radiobutton(bg_frame, text="Выкл", variable=self.widgets['noise_var'], value=0)\
           .pack(side=tk.LEFT, padx=(10, 0))

    def create_commands_column(self, parent):
        """Колонка команд"""
        # Capacitor
        ttk.Label(parent, text="Конденсатор:").grid(row=0, column=0, sticky=tk.W, pady=(0, 5))
        self.widgets['cond_combo'] = ttk.Combobox(parent, 
                                                 values=list(self.config.CAPACITOR_OPTIONS.keys()),
                                                 width=15, state="readonly")
        self.widgets['cond_combo'].grid(row=1, column=0, sticky=tk.W, pady=(0, 10))
        self.widgets['cond_combo'].set(self.config.DEFAULTS['capacitor'])
        
        # Apply parameters
        ttk.Button(parent, text="Применить параметры", 
                  command=self.send_config_command, width=20).grid(row=2, column=0, sticky=tk.W, pady=(0, 10))
        
        # Single read
        ttk.Button(parent, text="Одиночное чтение", width=20,
                  command=lambda: self.send_simple_command(self.config.COMMAND_CODES['send']))\
                  .grid(row=3, column=0, sticky=tk.W, pady=(0, 10))
        
        # Auto read
        ttk.Label(parent, text="Автоматическое чтение:").grid(row=4, column=0, sticky=tk.W, pady=(0, 5))
        
        auto_frame = ttk.Frame(parent)
        auto_frame.grid(row=5, column=0, sticky=tk.W, pady=(0, 5))
        
        ttk.Label(auto_frame, text="Интервал (мс):").pack(side=tk.LEFT)
        self.widgets['auto_entry'] = ttk.Entry(auto_frame, width=8)
        self.widgets['auto_entry'].pack(side=tk.LEFT, padx=(5, 10))
        self.widgets['auto_entry'].insert(0, self.config.DEFAULTS['auto_interval'])
        
        self.widgets['auto_btn'] = ttk.Button(parent, text="Пуск", 
                                            command=self.toggle_auto_read, 
                                            state=tk.DISABLED, width=10)
        self.widgets['auto_btn'].grid(row=6, column=0, sticky=tk.W)

    def create_gravity_column(self, parent):
        """Колонка центра тяжести и записи"""
        # Center of gravity display
        self.widgets['gravity_value'] = tk.Label(
            parent,
            text="0.00",
            font=('Arial', 20, 'bold'),
            foreground="blue",
            bg="white",
            relief="ridge",
            bd=2,
            padx=20,
            pady=10
        )
        self.widgets['gravity_value'].pack(fill=tk.BOTH, expand=True, pady=(0, 5))
        
        # Check word label
        self.widgets['check_word_label'] = tk.Label(
            parent,
            text="Проверка: ---",
            font=('Arial', 10, 'bold'),
            foreground="gray",
            bg="white",
            relief="sunken",
            bd=1,
            padx=10,
            pady=5
        )
        self.widgets['check_word_label'].pack(fill=tk.X, pady=(0, 10))
        
        # Log recording
        ttk.Label(parent, text="Запись в файл:", font=('Arial', 10)).pack(anchor=tk.W, pady=(5, 0))
        
        log_frame = ttk.Frame(parent)
        log_frame.pack(fill=tk.X, pady=(5, 0))
        
        self.widgets['log_var'] = tk.IntVar(value=0)
        ttk.Radiobutton(log_frame, text="Вкл", variable=self.widgets['log_var'], 
                        value=1, command=self.toggle_log_file).pack(side=tk.LEFT)
        ttk.Radiobutton(log_frame, text="Выкл", variable=self.widgets['log_var'], 
                        value=0, command=self.toggle_log_file).pack(side=tk.LEFT, padx=(10, 0))

    def create_plot_frame(self, parent):
        """Создание фрейма с графиком"""
        plot_frame = ttk.Frame(parent)
        plot_frame.grid(row=3, column=0, columnspan=3, sticky=(tk.W, tk.E, tk.N, tk.S), 
                       padx=10, pady=10)
        
        # Create matplotlib figure
        self.fig = Figure(figsize=(12, 6), dpi=100)
        self.canvas = FigureCanvasTkAgg(self.fig, master=plot_frame)
        self.canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True)

    def create_log_tab_content(self, parent):
        """Создание вкладки лога и команд"""
        main_frame = ttk.Frame(parent)
        main_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)
        
        # Log frame
        log_frame = ttk.LabelFrame(main_frame, text="Лог сообщений", padding="10", takefocus=0)
        log_frame.pack(fill=tk.BOTH, expand=True, pady=(0, 10))
        
        self.widgets['log_text'] = ScrolledText(log_frame, height=20, state='normal', wrap=tk.WORD)
        self.widgets['log_text'].pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        
        # Scrollbars
        scrollbar_y = ttk.Scrollbar(log_frame, orient=tk.VERTICAL, 
                                   command=self.widgets['log_text'].yview)
        scrollbar_y.pack(side=tk.RIGHT, fill=tk.Y)
        self.widgets['log_text'].config(yscrollcommand=scrollbar_y.set)
        
        scrollbar_x = ttk.Scrollbar(log_frame, orient=tk.HORIZONTAL,
                                   command=self.widgets['log_text'].xview)
        scrollbar_x.pack(side=tk.BOTTOM, fill=tk.X)
        self.widgets['log_text'].config(xscrollcommand=scrollbar_x.set)
            # ДОБАВЬ ЭТО: Привязываем Ctrl+C к копированию
        self.widgets['log_text'].bind('<Control-c>', self.copy_log_selected)
        # Command frame
        command_frame = ttk.LabelFrame(main_frame, text="Ручная отправка команд", 
                                      padding="10", takefocus=0)
        command_frame.pack(fill=tk.X, pady=(0, 10))
        
        ttk.Label(command_frame, text="Команда:").grid(row=0, column=0, padx=(0, 5), sticky=tk.W)
        
        self.widgets['command_entry'] = ttk.Entry(command_frame, width=50)
        self.widgets['command_entry'].grid(row=0, column=1, padx=(0, 10), sticky=(tk.W, tk.E))
        self.widgets['command_entry'].bind("<Return>", self.send_custom_command)
        
        ttk.Button(command_frame, text="Отправить", 
                  command=self.send_custom_command, width=12).grid(row=0, column=2, padx=(0, 10))
        
        ttk.Button(command_frame, text="Очистить лог", 
                  command=self.clear_log, width=12).grid(row=0, column=3)

    def setup_main_layout(self, parent):
        """Настройка разметки основной вкладки"""
        for i in range(3):
            parent.columnconfigure(i, weight=1)
        
        # Распределение весов строк
        parent.rowconfigure(0, weight=0)  # Заголовок (если будет)
        parent.rowconfigure(1, weight=0)  # Панель управления (фиксированная высота)
        parent.rowconfigure(2, weight=0)  # Разделитель (фиксированная высота)
        parent.rowconfigure(3, weight=1)  # График (растягивается)




    def copy_log_selected(self, event=None):
        """Копирование выделенного текста из лога (Ctrl+C)"""
        try:
            # Пытаемся получить выделенный текст
            selected = self.widgets['log_text'].get(tk.SEL_FIRST, tk.SEL_LAST)
            if selected:
                self.root.clipboard_clear()
                self.root.clipboard_append(selected)
                self.log_message("✓ Выделенный текст скопирован")
            else:
                # Если ничего не выделено, копируем весь лог
                all_text = self.widgets['log_text'].get(1.0, tk.END)
                if all_text.strip():
                    self.root.clipboard_clear()
                    self.root.clipboard_append(all_text)
                    self.log_message("✓ Весь лог скопирован")
        except tk.TclError:
            # Нет выделенного текста
            pass
        except Exception as e:
            self.log_message(f"✗ Ошибка копирования: {e}")
        return "break"  # Предотвращаем дальнейшую обработку
    # ========== ГРАФИК ==========
    
    def setup_plot(self):
        """Настройка bar plot"""
        self.ax = self.fig.add_subplot(111)
        self.ax_right = self.ax.twinx()
        
    # Connect events
        self.canvas.mpl_connect("motion_notify_event", self.on_hover)
        self.canvas.mpl_connect("axes_leave_event", self.on_axes_leave)
        self.canvas.mpl_connect("figure_leave_event", self.on_axes_leave)  # Добавим еще обработчик

        x_positions = np.arange(128)
        self.bars = self.ax.bar(x_positions, self.data_buffer, 
                                color='skyblue', edgecolor='black', alpha=0.8)
        
        # # Annotation for hover
        # self.annot = self.ax.annotate("", xy=(0,0), xytext=(20,20),
        #                             textcoords="offset points",
        #                             bbox=dict(boxstyle="round", 
        #                                     fc="white", 
        #                                     ec="gray",
        #                                     alpha=0.9,
        #                                     lw=1),
        #                             fontsize=9,
        #                             family='monospace')
        # self.annot.set_visible(False)
        self.current_highlighted = None
        
        # Plot appearance
        self.ax.set_xlabel('Номер канала', fontsize=12)
        # self.ax.set_ylabel('Относительное значение', fontsize=12, color='blue')
        self.ax.set_ylabel('Попугаи', fontsize=12, color='blue')

        self.ax.set_xlim(-1, 129)
        self.ax.set_ylim(0, 65536)
        self.ax.grid(True, alpha=0.3, linestyle='--')
        
        # Right axis (current)
        self.ax_right.set_ylabel('Ток, нА', fontsize=12, color='red')
        self.ax.tick_params(axis='y', labelcolor='blue')
        self.ax_right.tick_params(axis='y', labelcolor='red')
        # Линия центра тяжести (изначально скрыта)
        self.center_line = self.ax.axvline(x=64.5, color='red', linestyle='--', 
                                        linewidth=1.5, alpha=0.7)
        self.center_line.set_visible(False)
        
        # Adjustments
        self.fig.subplots_adjust(left=0.05, right=0.90, top=0.95, bottom=0.15)
        self.ax.set_xticks(np.arange(0, 128, 4))
        self.ax.set_xticklabels([str(i+1) for i in range(0, 128, 4)], 
                                rotation=0, fontsize=8, ha='right')
        self.ax.set_xticks(np.arange(0, 128, 2), minor=True)
        
        # Add parrot image
        try:
            img_array = self.utils.create_matplotlib_image_from_base64(self.config.ICONS["parrot"])
            if img_array is not None:
                img_ax = self.fig.add_axes([0, 0.17, 0.05, 0.05])
                img_ax.imshow(img_array)
                img_ax.axis('off')
        except Exception as e:
            print(f"Не удалось загрузить картинку: {e}")
        
        # Connect events
        self.canvas.mpl_connect("motion_notify_event", self.on_hover)
        # self.canvas.mpl_connect("figure_leave_event", lambda e: self.hide_annotation())
        
        # Для отладки - раскомментируйте при необходимости
        # self.canvas.mpl_connect("motion_notify_event", self.debug_mouse_events)
        
        self.update_current_scale()
        self.canvas.draw()

    def update_center_line(self):
        """Обновление линии центра тяжести на графике"""
        if self.center_of_gravity is not None:
            # Корректируем положение (график индексируется с 0)
            center_x = self.center_of_gravity - 0.5
            self.center_line.set_xdata([center_x, center_x])
            
            # Меняем цвет в зависимости от положения
            diff = abs(self.center_of_gravity - 64.5)
            if diff < 2:
                color = 'green'
            elif diff < 5:
                color = 'lime'
            elif diff < 10:
                color = 'orange'
            else:
                color = 'red'
                
            self.center_line.set_color(color)
            self.center_line.set_visible(True)
        else:
            self.center_line.set_visible(False)

    # В методе on_hover добавьте проверку:
    def on_hover(self, event):
        """Обработка наведения мыши на столбцы"""
        # Проверяем, находится ли курсор над любой из осей графика
        if event.inaxes is None or (event.inaxes != self.ax and event.inaxes != self.ax_right):
            # self.hide_annotation()
            self.clear_selected_bar_display()
            self.highlight_bar(None)
            return
        
        x = event.xdata
        if x is None:
            # self.hide_annotation()
            self.clear_selected_bar_display()
            self.highlight_bar(None)
            return
        
        # Находим индекс столбца
        # Столбцы расположены в позициях 0, 1, 2, ... 127
        # Центр каждого столбца находится в целых координатах
        bar_index = int(round(x))
        
        # Ограничиваем индекс диапазоном 0-127
        bar_index = max(0, min(127, bar_index))
        
        # Получаем столбец и его высоту
        bar = self.bars[bar_index]
        height = bar.get_height()
        
        # Обновляем отображение в колонке
        self.update_selected_bar_display(bar_index, height)
        
        # Обновляем всплывающую аннотацию
        # self.update_annotation(bar_index, height, event)
        
        # Подсвечиваем столбец
        self.highlight_bar(bar_index)

    def on_axes_leave(self, event):
        """Обработка ухода мыши с осей"""
        # self.hide_annotation()
        self.clear_selected_bar_display()
        self.highlight_bar(None)

    def update_selected_bar_display(self, bar_index, height):
        """Обновление отображения выбранного столбца в отдельной колонке"""
        try:
            # Проверяем, что виджеты созданы
            if not hasattr(self, 'widgets'):
                return
                
            # Обновляем номер канала
            if 'selected_channel' in self.widgets:
                self.widgets['selected_channel'].config(text=str(bar_index + 1))
            
            # Обновляем значение в относительных единицах
            if 'selected_value' in self.widgets:
                self.widgets['selected_value'].config(text=f"{int(height)}")
            
            # Обновляем ток для выбранного канала
            if 'selected_current' in self.widgets:
                current_coeff = self.calculate_current_coefficient()
                if current_coeff is not None and current_coeff > 0:
                    current_nA = (height - 256) * current_coeff
                    sign = "+" if current_nA >= 0 else ""
                    self.widgets['selected_current'].config(text=f"{sign}{current_nA:.6f} нА")
                else:
                    self.widgets['selected_current'].config(text="--- нА")
            
            # Обновляем общий ток
            self.update_total_current_display()
                    
        except Exception as e:
            print(f"Ошибка при обновлении отображения: {e}")    

    def update_total_current_display(self):
        """Обновление отображения общего тока"""
        try:
            if 'total_current' not in self.widgets:
                return
                
            total_current = self.calculate_total_current()
            if total_current is not None:
                sign = "+" if total_current >= 0 else ""
                self.widgets['total_current'].config(text=f"{sign}{total_current:.6f} нА")
            else:
                self.widgets['total_current'].config(text="--- нА")
        except Exception as e:
            print(f"Ошибка при обновлении общего тока: {e}")

    def clear_selected_bar_display(self):
        """Очистка отображения выбранного столбца"""
        try:
            if 'selected_channel' in self.widgets:
                self.widgets['selected_channel'].config(text="---")
            if 'selected_value' in self.widgets:
                self.widgets['selected_value'].config(text="---")
            if 'selected_current' in self.widgets:
                self.widgets['selected_current'].config(text="--- нА")
        except Exception as e:
            print(f"Ошибка при очистке отображения: {e}")

    def update_annotation(self, bar_index, height, event):
        """Обновление текста и позиции аннотации"""
        try:
            current_coeff = self.calculate_current_coefficient()
            
            if current_coeff is not None and current_coeff > 0:
                current_nA = height * current_coeff
                self.annot.set_text(f"Канал: {bar_index + 1}\nЗначение: {height:.1f}\nТок: {current_nA:.2f} нА")
            else:
                self.annot.set_text(f"Канал: {bar_index + 1}\nЗначение: {height:.1f}")
            
            bar = self.bars[bar_index]
            bar_center_x = (bar.get_x() + bar.get_width() / 2)
            bar_top_y = bar.get_height()
            
            # Позиционируем аннотацию
            self.annot.xy = (bar_center_x, bar_top_y)
            
            # Определяем положение аннотации (слева или справа от столбца)
            if event.xdata > 64:
                self.annot.set_position((-120, 20))
                self.annot.set_ha("right")
            else:
                self.annot.set_position((20, 20))
                self.annot.set_ha("left")
            
            if not self.annot.get_visible():
                self.annot.set_visible(True)
                self.canvas.draw_idle()
                
        except Exception as e:
            print(f"Ошибка при обновлении аннотации: {e}")

    def hide_annotation(self):
        """Скрытие аннотации"""
        if self.annot.get_visible():
            self.annot.set_visible(False)
            self.canvas.draw_idle()

    def highlight_bar(self, bar_index):
        """Подсветка указанного столбца"""
        try:
            # Проверяем, что bars существует
            if not hasattr(self, 'bars') or self.bars is None:
                return
                
            # Сначала сбрасываем все столбцы к исходному виду
            for i, bar in enumerate(self.bars):
                if i == self.current_highlighted:
                    continue  # Пропускаем уже подсвеченный
                bar.set_color('skyblue')
                bar.set_alpha(0.8)
                bar.set_edgecolor('black')
                bar.set_linewidth(1.0)
            
            # Если bar_index None, просто сбрасываем подсветку
            if bar_index is None:
                if self.current_highlighted is not None:
                    old_bar = self.bars[self.current_highlighted]
                    old_bar.set_color('skyblue')
                    old_bar.set_alpha(0.8)
                    old_bar.set_edgecolor('black')
                    old_bar.set_linewidth(1.0)
                self.current_highlighted = None
                
            # Подсвечиваем новый столбец
            elif 0 <= bar_index < 128:
                # Сбрасываем предыдущую подсветку
                if self.current_highlighted is not None and self.current_highlighted != bar_index:
                    old_bar = self.bars[self.current_highlighted]
                    old_bar.set_color('skyblue')
                    old_bar.set_alpha(0.8)
                    old_bar.set_edgecolor('black')
                    old_bar.set_linewidth(1.0)
                
                # Подсвечиваем новый
                bar = self.bars[bar_index]
                bar.set_color('#ff9900')  # Оранжевый цвет
                bar.set_alpha(1.0)
                bar.set_edgecolor('black')
                bar.set_linewidth(2.0)
                self.current_highlighted = bar_index
            
            if hasattr(self, 'canvas'):
                self.canvas.draw_idle()
            
        except Exception as e:
            print(f"Ошибка при подсветке столбца: {e}")
    def update_selected_bar_display_from_data(self):
        """Обновление отображения выбранного столбца из текущих данных"""
        if self.current_highlighted is not None and 0 <= self.current_highlighted < 128:
            height = self.data_buffer[self.current_highlighted]
            self.update_selected_bar_display(self.current_highlighted, height)
    # ========== РАСЧЕТ ТОКА ==========
    

    def calculate_total_current(self):
        """Расчет общего тока (суммы по всем каналам)"""
        try:
            current_coeff = self.calculate_current_coefficient()
            if current_coeff is None or current_coeff <= 0:
                return None
            
            # Суммируем токи по всем каналам (с учетом смещения 256)
            total_current_nA = 0.0
            for value in self.data_buffer:
                current_nA = (value - 256) * current_coeff
                total_current_nA += current_nA
            
            return total_current_nA
            
        except Exception as e:
            self.log_message(f"Ошибка расчета общего тока: {e}")
            return None



    def calculate_current_coefficient(self):
        """Расчет коэффициента для преобразования в ток"""
        try:
            capacitor_str = self.widgets['cond_combo'].get()
            max_charge_pC = self.config.CAPACITOR_VALUES_PC.get(capacitor_str)
            
            if max_charge_pC is None:
                return None
            
            conversion_interval_us = float(self.widgets['conv_entry'].get())
            conversion_interval_s = conversion_interval_us * 1e-6
            max_charge_C = max_charge_pC * 1e-12
            max_current = max_charge_C / conversion_interval_s
            current_per_count = max_current / 65535.0
            return current_per_count * 1e9  # наноамперы
            
        except Exception as e:
            self.log_message(f"Ошибка расчета тока: {e}")
            return None

    def update_current_scale(self):
        """Обновление шкалы тока"""
        ylim = self.ax.get_ylim()
        current_coeff = self.calculate_current_coefficient()
        
        if current_coeff is not None:
            # Ключевое изменение: пересчитываем с поправкой на нуль (256)
            bottom_current = (ylim[0] - 256) * current_coeff
            top_current = (ylim[1] - 256) * current_coeff
            self.ax_right.set_ylim(bottom_current, top_current)
            
            # Добавляем линию нуля
            self.ax_right.axhline(y=0, color='gray', linestyle='-', linewidth=0.5, alpha=0.5)
            
            capacitor_str = self.widgets['cond_combo'].get()
            self.ax_right.set_ylabel(f'Ток, нА ({capacitor_str})', fontsize=12, color='red')
            self.canvas.draw_idle()

    # ========== ПОСЛЕДОВАТЕЛЬНЫЙ ПОРТ ==========
    
    def refresh_ports(self):
        """Обновление списка доступных COM-портов"""
        ports = [port.device for port in serial.tools.list_ports.comports()]
        self.widgets['port_combo']['values'] = ports
        if ports:
            self.widgets['port_combo'].set(ports[0])
            self.log_message(f"Найдено портов: {len(ports)}")

    def toggle_connection(self):
        """Подключение/отключение от COM порта"""
        if not self.is_connected:
            self.connect_serial()
        else:
            self.disconnect_serial()

    def connect_serial(self):
        """Подключение к выбранному COM-порту"""
        port = self.widgets['port_combo'].get()
        baud = int(self.widgets['baud_combo'].get())
        
        if not port:
            messagebox.showerror("Ошибка", "Выберите COM порт!")
            return
            
        try:
            self.serial_port = serial.Serial(
                port=port,
                baudrate=baud,
                timeout=1,
                write_timeout=1
            )
            
            self.is_connected = True
            self.widgets['connect_btn'].config(text="Отключиться")
            self.widgets['status_label'].config(text="Подключено", foreground="green")
            self.widgets['auto_btn'].config(state=tk.NORMAL)
            
            thread = threading.Thread(target=self.read_serial_data, daemon=True)
            thread.start()
            
            self.log_message(f"✓ Подключено к {port} ({baud} бод)")
            
        except SerialException as e:
            messagebox.showerror("Ошибка подключения", 
                                f"Не удалось подключиться к {port}:\n{str(e)}")
        except Exception as e:
            messagebox.showerror("Ошибка", f"Непредвиденная ошибка:\n{str(e)}")

    def disconnect_serial(self):
        """Отключение от COM-порта"""
        self.is_connected = False
        self.last_command = None  # ДОБАВИТЬ ЭТУ СТРОКУ
        
        if self.is_auto_reading:
            self.stop_auto_read()
            
        if self.serial_port and self.serial_port.is_open:
            try:
                self.serial_port.close()
                self.log_message("✗ Отключено от COM порта")
            except Exception as e:
                self.log_message(f"Ошибка при отключении: {e}")
        
        self.widgets['connect_btn'].config(text="Подключиться")
        self.widgets['status_label'].config(text="Отключено", foreground="red")
        self.widgets['auto_btn'].config(state=tk.DISABLED)

    @handle_serial_errors
    def send_simple_command(self, command_code):
        """Отправка простой команды (однобайтовой)"""
        if command_code == 0x45:
            self.last_command = 'send'
        elif command_code == 0x15:
            self.last_command = 'accumulate'
            
        arr = [0]*10
        arr[0] = command_code
        hex_command = ' '.join(f'{byte:02X}' for byte in arr)
        self.log_message(f"Команда (hex): {hex_command}")
        self.send_command(bytes(arr))

    @validate_connection
    def send_config_command(self):
        """Отправка команды конфигурации с текущими параметрами"""
        self.last_command = 'config'  # ДОБАВИТЬ ЭТУ СТРОКУ
        try:
            # Reset parameter byte
            self.command_params[1] = 0
            
            # Process settings
            self.process_capacitor()
            
            if self.widgets['avg_var'].get():
                self.command_params[1] |= 1 << 3
                
            if self.widgets['noise_var'].get():
                self.command_params[1] |= 1 << 4
                
            self.process_trigger()
            
            # Get values
            conv_interval = int(self.widgets['conv_entry'].get()) * 2
            avg_cycles = int(self.widgets['it_entry'].get())
            trigger_level = int(self.widgets['trigger_entry'].get())
            
            # Convert to bytes
            conv_bytes = 0
            conv_bytes = conv_interval.to_bytes(4, 'little')
            avg_bytes = avg_cycles.to_bytes(4, 'little')
            trig_bytes = trigger_level.to_bytes(4, 'little')
            
            # Build command
            command = [
                self.config.COMMAND_CODES['config'],
                self.command_params[1],
                conv_bytes[3], conv_bytes[2],
                conv_bytes[1], conv_bytes[0],
                avg_bytes[0], avg_bytes[1],
                trig_bytes[0], trig_bytes[1]
            ]
            hex_command = ' '.join(f'{byte:02X}' for byte in command)
            self.log_message(f"Команда (hex): {hex_command}")
            self.send_command(command)
            # self.log_message("✓ Конфигурация отправлена")
            self.update_current_scale()
            
        except ValueError as e:
            messagebox.showerror("Ошибка ввода", 
                                f"Проверьте корректность числовых значений:\n{str(e)}")
        except Exception as e:
            self.log_message(f"✗ Ошибка отправки конфигурации: {e}")

    def process_capacitor(self):
        """Обработка выбора конденсатора"""
        selection = self.widgets['cond_combo'].get()
        value = self.config.CAPACITOR_OPTIONS.get(selection, 0)
        self.command_params[1] |= value

    def process_trigger(self):
        """Обработка настроек триггера"""
        if self.widgets['pulse_var'].get():
            trigger_type = self.widgets['trigger_combo'].get()
            
            trigger_mapping = {
                'Аппаратный': 1 << 6,
                'Программный': 1 << 5
            }
            
            if trigger_type in trigger_mapping:
                self.command_params[1] |= trigger_mapping[trigger_type]

    @handle_serial_errors
    def send_command(self, command):
        """Универсальный метод отправки команды в COM-порт"""
        # Handle different command types
        command_mapping = {
            list: lambda x: bytes(x),
            bytes: lambda x: x,
            str: lambda x: x.encode(),
            int: lambda x: bytes([x])
        }
        
        for cmd_type, converter in command_mapping.items():
            if isinstance(command, cmd_type):
                data_bytes = converter(command)
                break
        else:
            self.log_message(f"✗ Неверный тип команды: {type(command)}")
            return False
        
        full_command = data_bytes #+ b'\n'
        self.serial_port.write(full_command)
        return True


    def read_serial_data(self):
        """Чтение данных из COM-порта"""
        buffer = bytearray()
        
        while self.is_connected:
            try:
                if not self.serial_port or not self.serial_port.is_open:
                    break
                    
                if self.serial_port.in_waiting:
                    data = self.serial_port.read(self.serial_port.in_waiting)
                    buffer.extend(data)
                    
                    # Если режим бинарного чтения
                    if self.last_command == 'send':
                        while len(buffer) >= 262:
                            packet = buffer[:262]
                            buffer = buffer[262:]
                            self.process_binary_packet(packet)
                    else:
                        # INFO сообщения
                        while b'\n' in buffer:
                            line, buffer = buffer.split(b'\n', 1)
                            line = line.strip()
                            if line:
                                try:
                                    text = line.decode('utf-8', errors='ignore')
                                    if '[INFO]' in text:
                                        self.process_info_message(text)
                                except:
                                    pass
                                            
            except Exception as e:
                self.log_message(f"✗ Ошибка: {e}")
                continue

    def process_info_message(self, line):
        """Обработка INFO сообщений"""
        info_msg = line.replace('[INFO]', '').strip()
        self.log_message(f"ℹ {info_msg}")

    def process_binary_packet(self, packet):
        """Обработка бинарного пакета от команды 0x45"""
        try:
            channel_data = list(struct.unpack('<128H', packet[:256]))
            self.center_of_gravity = struct.unpack('<f', packet[256:260])[0]
            self.extra_value = struct.unpack('<H', packet[260:262])[0]
            
            # Отладка
            self.log_message(f"ЦТ: {self.center_of_gravity:.2f}, CW: 0x{self.extra_value:04X}")
            
            self.root.after(0, self.update_gravity_display)
            self.root.after(0, self.update_check_word_display)
            self.data_queue.put(channel_data)
            
            # НЕ сбрасываем last_command при авточтении!
            if not self.is_auto_reading:
                self.last_command = None
            
        except Exception as e:
            self.log_message(f"✗ Ошибка обработки: {e}")
            self.last_command = None



    def send_custom_command(self, event=None):
        """Отправка пользовательской команды из поля ввода"""
        command = self.widgets['command_entry'].get().strip()
        if command:
            # self.send_command(command)
            self.widgets['command_entry'].delete(0, tk.END)

    # ========== АВТОЧТЕНИЕ ==========
    
    def toggle_auto_read(self):
        """Включение/выключение автоматического чтения"""
        if not self.is_auto_reading:
            self.start_auto_read()
        else:
            self.stop_auto_read()

    def start_auto_read(self):
        """Запуск автоматического чтения"""
        try:
            interval = max(50, int(self.widgets['auto_entry'].get()))
            self.widgets['auto_entry'].delete(0, tk.END)
            self.widgets['auto_entry'].insert(0, str(interval))
                
            self.is_auto_reading = True
            self.widgets['auto_btn'].config(text="Стоп")
            self.widgets['auto_entry'].config(state=tk.DISABLED)
            
            self.log_message(f"▶ Авточтение запущено ({interval} мс)")
            self.auto_read_loop(interval)
            
        except ValueError:
            messagebox.showerror("Ошибка", "Введите корректный интервал (мс)")

    def stop_auto_read(self):
        """Остановка автоматического чтения"""
        self.is_auto_reading = False
        self.last_command = None  # Сбрасываем флаг
        self.widgets['auto_btn'].config(text="Пуск")
        self.widgets['auto_entry'].config(state=tk.NORMAL)
        
        if self.auto_timer:
            self.root.after_cancel(self.auto_timer)
            self.auto_timer = None
            
        self.log_message("■ Авточтение остановлено")

    def auto_read_loop(self, interval):
        """Цикл автоматического чтения"""
        if self.is_auto_reading and self.is_connected:
            self.send_simple_command(self.config.COMMAND_CODES['send'])
            self.auto_timer = self.root.after(interval, 
                                            lambda: self.auto_read_loop(interval))

    # ========== ОБРАБОТКА ДАННЫХ ==========
    
    

    

    

    # УПРОСТИТЬ ДО ТАКОГО ВИДА:
    def process_complete_data(self):
        """Обработка полного набора данных (упрощенно для бинарного протокола)"""
        # Просто сбрасываем временный буфер
        self.temp_data_buffer = []

    def update_gravity_display(self):
        """Обновление отображения центра тяжести"""
        if 'gravity_value' in self.widgets:
            self.widgets['gravity_value'].config(text=f"{self.center_of_gravity:.2f}")
            
            # Цвет в зависимости от значения
            diff = abs(self.center_of_gravity - 64.5)
            colors = {
                diff > 32: "red",
                diff > 16: "orange",
                True: "green"
            }
            
            for condition, color in colors.items():
                if condition:
                    self.widgets['gravity_value'].config(foreground=color)
                    break

    def update_check_word_display(self):
        """Обновление отображения проверочного слова"""
        if 'check_word_label' in self.widgets:
            if self.extra_value is not None:
                try:
                    check_value = int(self.extra_value)
                    
                    if check_value == self.config.CHECK_WORD_EXPECTED:
                        text = f"Проверка: 0x{check_value:04X} ✓"
                        color = "green"
                    else:
                        text = f"Проверка: 0x{check_value:04X} ≠ 0x{self.config.CHECK_WORD_EXPECTED:04X}"
                        color = "red"
                        
                    self.widgets['check_word_label'].config(text=text, foreground=color)
                    
                except (ValueError, TypeError):
                    self.widgets['check_word_label'].config(
                        text=f"Проверка: {self.extra_value}",
                        foreground="orange"
                    )
            # else:
            #     self.widgets['check_word_label'].config(
            #         text="Проверка: ---",
            #         foreground="gray"
            #     )

    def update_data(self):
        """Обновление данных на графике из очереди"""
        try:
            while not self.data_queue.empty():
                new_data = self.data_queue.get_nowait()
                
                if len(new_data) == 128:
                    self.update_plot_data(new_data)
                        
        except queue.Empty:
            pass
                
        self.root.after(50, self.update_data)

    def update_plot_data(self, new_data):
        """Обновление данных на графике"""
        self.data_buffer = np.array(new_data)
        # self.data_buffer[10]=43924
        # self.data_buffer[11]=43946
        # self.data_buffer[10]=255
        # self.data_buffer[11]=43946
        # Update bars
        for bar, height in zip(self.bars, self.data_buffer):
            bar.set_height(height)
        
        # Smart scaling
        if len(self.data_buffer) > 0:
            max_val = max(self.data_buffer)
            min_val = min(self.data_buffer)
            data_range = max_val - min_val
            
            if max_val > 0:
                if data_range > 0:
                    margin = data_range * 0.05
                    bottom = max(0, min_val - margin)
                    top = max_val + margin
                else:
                    bottom = 0
                    top = max_val * 1.1
                
                self.ax.set_ylim(bottom, top)
                
                # Update current scale
                current_coeff = self.calculate_current_coefficient()
                if current_coeff is not None:
                    bottom_current = bottom * current_coeff
                    top_current = top * current_coeff
                    self.ax_right.set_ylim(bottom_current, top_current)
            else:
                self.ax.set_ylim(0, 10)
                self.ax_right.set_ylim(0, 10)
         # ЕСЛИ ВКЛЮЧЕНА ЗАПИСЬ - СОХРАНЯЕМ ДАННЫЕ
        if self.log_enabled:
            self.write_current_data_to_log()
    
        # ОБНОВЛЕНИЕ: Обновляем информацию о текущем выделенном канале
        if self.current_highlighted is not None and 0 <= self.current_highlighted < 128:
            current_height = self.data_buffer[self.current_highlighted]
            self.update_selected_bar_display(self.current_highlighted, current_height)
            # Подсвечиваем столбец снова после обновления данных
            self.highlight_bar(self.current_highlighted)
        
        # Update gravity display
        self.update_gravity_display()

        # Update center of gravity line
        self.update_center_line()

        # В методе update_plot_data добавьте эту строку перед self.canvas.draw()
        self.update_total_current_display()
        self.canvas.draw()

    def start_resize(self, event):
        """Начало изменения размера"""
        self.is_resizing = True
        self.resize_start_y = event.y_root
        self.resize_start_height = self.commands_height
        # Визуальная обратная связь
        self.separator.config(style='Trough.Horizontal.TSeparator')  # Стиль для активного состояния

    def stop_resize(self, event):
        """Завершение изменения размера"""
        self.is_resizing = False
        # Возвращаем обычный стиль
        self.separator.config(style='Horizontal.TSeparator')
        
    def do_resize(self, event):
        """Изменение размера при перетаскивании"""
        if not self.is_resizing:
            return
        
        # Вычисляем изменение высоты
        delta_y = event.y_root - self.resize_start_y
        
        # Минимальная и максимальная высота панели управления
        min_height = 150
        max_height = 600
        
        # Вычисляем новую высоту
        new_height = self.resize_start_height + delta_y
        new_height = max(min_height, min(max_height, new_height))
        
        # Обновляем высоту панели управления только если она изменилась
        if new_height != self.commands_height:
            self.commands_height = new_height
            self.widgets['commands_frame'].config(height=new_height)
            
            # Обновляем положение разделителя и графика
            self.update_layout()

        
    def update_layout(self):
        """Обновление разметки после изменения размера"""
        # Обновляем высоту фрейма команд
        self.widgets['commands_frame'].config(height=self.commands_height)
        
        # Пересчитываем размеры в канвасе команд
        if 'commands_canvas' in self.widgets:
            canvas = self.widgets['commands_canvas']
            inner_frame = self.widgets['commands_inner_frame']
            
            # Обновляем область прокрутки
            canvas.config(scrollregion=canvas.bbox("all"))
            
            # Проверяем, нужна ли вертикальная прокрутка
            inner_height = inner_frame.winfo_reqheight()
            canvas_height = canvas.winfo_height()
            
            # Показываем или скрываем вертикальную прокрутку
            if inner_height > canvas_height:
                self.widgets['v_scrollbar'].grid()
            else:
                self.widgets['v_scrollbar'].grid_remove()
        
        # Обновляем график
        self.canvas.draw_idle()
        
        # Принудительно обновляем геометрию окна
        self.widgets['commands_frame'].update_idletasks()
    # ========== ЛОГИРОВАНИЕ ==========
    
    def toggle_log_file(self):
        """Включение/выключение записи в лог-файл"""
        if self.widgets['log_var'].get() == 1:
            # Включаем запись
            self.start_logging()
        else:
            # Выключаем запись
            self.stop_logging()

    def start_logging(self):
        """Начать запись в лог-файл (создает новый CSV файл)"""
        try:
            if getattr(sys, 'frozen', False):
                exe_dir = os.path.dirname(sys.executable)
            else:
                exe_dir = os.path.dirname(os.path.abspath(__file__))
            
            log_dir = os.path.join(exe_dir, "logs")
            if not os.path.exists(log_dir):
                os.makedirs(log_dir)
            
            timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
            self.log_filename = f"ddc_data_{timestamp}.csv"
            log_path = os.path.join(log_dir, self.log_filename)
            
            # Открываем файл для записи (чистый CSV, без комментариев)
            self.log_file = open(log_path, 'w', encoding='utf-8')
            self.log_enabled = True
            
            # НЕ пишем никаких комментариев в файл!
            
            self.log_message(f"✓ Запись в файл включена: {self.log_filename}")
            
        except Exception as e:
            self.log_message(f"✗ Ошибка создания лог-файла: {e}")
            self.widgets['log_var'].set(0)
            self.log_enabled = False

    def stop_logging(self):
        """Остановить запись в лог-файл"""
        if hasattr(self, 'log_file') and self.log_file and not self.log_file.closed:
            try:
                # Просто закрываем файл, без записи комментариев
                self.log_file.close()
                self.log_message("✓ Запись в файл остановлена")
            except Exception as e:
                print(f"Ошибка закрытия лог-файла: {e}")
        
        self.log_enabled = False
        self.log_file = None

    def write_current_data_to_log(self):
        """Запись текущих данных в CSV файл (только данные, без логов)"""
        if not self.log_enabled or self.log_file is None or self.log_file.closed:
            return
        
        try:
            # Только дата и время
            current_datetime = datetime.now().strftime('%Y-%m-%d %H:%M:%S.%f')[:-3]
            self.log_file.write(f"{current_datetime}\n")
            
            # 128 значений (каждое с новой строки)
            for value in self.data_buffer:
                self.log_file.write(f"{int(value)}\n")
            
            # Центр тяжести
            self.log_file.write(f"{self.center_of_gravity:.6f}\n")
            
            # Пустая строка-разделитель
            self.log_file.write(f"\n")
            self.log_file.flush()
            
        except Exception as e:
            self.log_message(f"✗ Ошибка записи в лог: {e}")

    def log_message(self, message):
        """Добавление сообщения в лог (ТОЛЬКО в интерфейс, НЕ в файл)"""
        timestamp = self.utils.get_timestamp()
        log_line = f"{timestamp} {message}"
        
        # Только в интерфейс
        if 'log_text' in self.widgets:
            self.widgets['log_text'].insert(tk.END, log_line + "\n")
            self.widgets['log_text'].see(tk.END)
            self.widgets['log_text'].update_idletasks()
        
        # НЕ ПИШЕМ В ФАЙЛ! Файл только для данных.

    def clear_log(self):
        """Очистка лога"""
        self.widgets['log_text'].delete(1.0, tk.END)
        self.log_message("Лог очищен")

    # ========== ЗАКРЫТИЕ ==========
    
    def on_closing(self):
        """Обработка закрытия окна"""
         # Останавливаем авточтение
        if self.is_auto_reading:
            self.stop_auto_read()
            
        # Останавливаем запись в файл
        self.stop_logging()
            
        # Отключаемся от порта
        self.disconnect_serial()
            
        # Закрываем окно
        self.root.destroy()    
    

# ========== ЗАПУСК ПРИЛОЖЕНИЯ ==========

def main():
    """Основная функция запуска приложения"""
    root = tk.Tk()
    app = SerialBarPlotApp(root)
    root.mainloop()


if __name__ == "__main__":
    main()