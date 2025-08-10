# Libft

Bu proje, C programlama dilinde standart kütüphane fonksiyonlarını yeniden yazarak kendi temel kütüphanemi oluşturmamı amaçlamaktadır. `libft` projesi, 42 okulu kapsamında yapılan ilk projelerden biridir ve yazdığım diğer projelerde tekrar kullanabileceğim işlevsel fonksiyonları içerir.

## 📁 İçerik

Bu projede aşağıdaki iki ana başlık altında fonksiyonlar geliştirilmiştir:

### 1. Libc Fonksiyonları

- Karakter kontrolleri: `ft_isalpha`, `ft_isdigit`, `ft_isalnum`, `ft_isascii`, `ft_isprint`
- Bellek işlemleri: `ft_memset`, `ft_bzero`, `ft_memcpy`, `ft_memmove`, `ft_memchr`, `ft_memcmp`
- String işlemleri: `ft_strlen`, `ft_strlcpy`, `ft_strlcat`, `ft_strchr`, `ft_strrchr`, `ft_strncmp`, `ft_strnstr`, `ft_strdup`
- Dönüştürme: `ft_atoi`, `ft_toupper`, `ft_tolower`
- Dinamik bellek: `ft_calloc`

### 2. Ek Fonksiyonlar

- String işlemleri: `ft_substr`, `ft_strjoin`, `ft_strtrim`, `ft_split`, `ft_strmapi`, `ft_striteri`
- Sayı/String dönüşümü: `ft_itoa`
- File descriptor üzerinden çıktı: `ft_putchar_fd`, `ft_putstr_fd`, `ft_putendl_fd`, `ft_putnbr_fd`

## ⚙️ Kurulum ve Derleme

Projeyi derlemek için `Makefile` hazır olarak sunulmuştur.

```bash
make
```

Temizlik işlemleri için:

```bash
make clean     # Nesne dosyalarını siler
make fclean    # Nesne dosyaları + libft.a dosyasını siler
make re        # Temizler ve yeniden derler
```

## 🛠 Kullanım

Derleme sonrası `libft.a` adlı bir statik kütüphane oluşur. Bu kütüphaneyi kendi projelerinizde aşağıdaki gibi kullanabilirsiniz:

```c
#include "libft.h"
```

Ve derlerken:

```bash
gcc main.c -L. -lft
```

## 📌 Notlar

- Bonus kısmı bu versiyona dahil edilmemiştir.
- Tüm fonksiyonlar `Norminette` kurallarına uygun olarak yazılmıştır.
- Her `.c` dosyası bağımsız şekilde `-Wall -Wextra -Werror` flagleriyle sorunsuz derlenebilir.
