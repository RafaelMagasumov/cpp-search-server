// Решите загадку: Сколько чисел от 1 до 1000 содержат как минимум одну цифру 3?
// Напишите ответ здесь:
int main() {
	int c = 0;
	vector <int> s;
	for (int i = 1; i < 1000; ++i) {
		if (i % 10 == 3 || i / 100 == 3 || (i / 10) % 10 == 3 ) {
			++c;
		}
	}
	cout << c;
}

// Закомитьте изменения и отправьте их в свой репозиторий.
